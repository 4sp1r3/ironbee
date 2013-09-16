--[[-------------------------------------------------------------------------
-- IronBee configuration code.
--]]-------------------------------------------------------------------------

local ffi = require('ffi')

local _M = {}
_M.__index = _M
_M._COPYRIGHT = "Copyright (C) 2010-2013 Qualys, Inc."
_M._DESCRIPTION = "IronBee Configuration"
_M._VERSION = "1.0"

local Waggle = require('ironbee/waggle')
local Predicate = require('ironbee/predicate')

-- Pair of #defines from C code imported here.
local IB_RULEMD_FLAG_EXPAND_MSG = 1
local IB_RULEMD_FLAG_EXPAND_DATA = 2

-- Pair of #defines from C code imported here.
local IB_OP_CAPABILITY_NON_STREAM = 2
local IB_OP_CAPABILITY_STREAM = 4

-- (IB_RULE_FLAG_FORCE_EN|IB_RULE_FLAG_NO_TGT) or (256 + 512)
local IB_RULE_FLAG_ACTION = 768

-- Setup the configuration DLS, run the function provided, tear down the DSL.
--
-- param[in] f Function to run after the DSL is installed in _G.
-- param[in] cp Configuration parser.
--
local DoInDSL = function(f, cp)
    local ib = ibapi.engineapi:new(ffi.cast("ib_cfgparser_t*", cp).ib)

    local SkipTable = {
        _DESCRIPTION = 1,
        _COPYRIGHT = 1,
        __index = 1,
        _VERSION = 1
    }

    local to_nil = {}

    -- Setup all symbols.
    for k,v in pairs(Waggle) do
        if not SkipTable[k] then
            _G[k] = v
            table.insert(to_nil, k)
        end
    end
    _G['P'] = Predicate
    _G['IB'] = ib
    _G['CP'] = cp

    f()

    -- Teardown.
    _G['P'] = nil
    _G['IB'] = nil
    _G['CP'] = nil
    for _, k in ipairs(to_nil) do
        _G[k] = nil
    end
end

-- Include a configuration file.
--
-- Rules are not committed to the engine until the end of the
-- configuration phase.
--
-- param[in] cp Configuraiton Parser.
-- param[in] file String point to the file to operate on.
_M.include = function(cp, file)
    local ib = ibapi.engineapi:new(ffi.cast("ib_cfgparser_t*", cp).ib)

    DoInDSL(function()
        ib:logInfo("Loading file %s", file)
        dofile(file)
        ib:logInfo("Done loading file %s", file)
    end, cp)

    return ffi.C.IB_OK
end

--
-- param[in] ib IronBee engine.
-- param[in] rule Lua rule table.
-- param[in] prule An ib_rule_t*[1].
-- param[in] field Lua field table in rule.data.fields.
--
local add_fields = function(ib, rule, prule, field)
    local str = field.collection
    local name = field.collection
    local not_found = ffi.new("int[1]")
    local target = ffi.new("ib_rule_target_t*[1]")
    local tfn_names = ffi.new("ib_list_t*[1]")
    if field.selector then
        str = str .. ":" .. field.selector
        name = name .. ":" .. field.selector
    end
    if field.transformation then
        str = str .. "." .. field.transformation
    end

    rc = ffi.C.ib_list_create(
        tfn_names,
        ffi.C.ib_engine_pool_main_get(ib.ib_engine))
    if rc ~= ffi.C.IB_OK then
        ib:logError("Failed to create new ib_list_t.")
        return rc
    end

    -- Build up transformation list.
    if field.transformation then
        -- Split and walk the transformation list.
        for tfn_name in string.gmatch(field.transformation, "([^.()]+)") do
            -- Add it to the list.
            rc = ffi.C.ib_list_push(tfn_names[0], ffi.cast("char*", tfn_name))
            if rc ~= ffi.C.IB_OK then
                ib:logError(
                    "Failed to add transformation %s to list.",
                    tfn_name)
                return rc
            end
        end
    end

    -- Create target
    rc = ffi.C.ib_rule_create_target(
        ib.ib_engine,
        str,
        tfn_names[0],
        target,
        not_found)
    if rc ~= ffi.C.IB_OK then
        ib:logError("Failed to create field %s", str)
        return rc
    end

    -- Add target.
    rc = ffi.C.ib_rule_add_target(
        ib.ib_engine,
        prule[0],
        target[0])
    if rc ~= ffi.C.IB_OK then
        ib:logError("Failed to add field %s to rule.", str)
        return rc
    end

    return ffi.C.IB_OK
end

-- Create, setup, and register a rule in the given ironbee engine.
-- @param[in] ib IronBee Engine.
-- @param[in] ctx The context the rules should be added to.
-- @param[in] chain A list of rule-result pairs.
--            Each element in this list is a table with
--            "rule" being the rule id and "result" being the
--            result required for the rule to allow the next rule
--            in the chain to fire.
--            The result is irrelevant in the last rule in the rule chain.
-- @param[in] db The database that contains the full rule definition.
local build_rule = function(ib, ctx, chain, db)
    for i, link in ipairs(chain) do
        local rule_id = link.rule
        local result = link.result
        local rule = db.db[rule_id]
        local prule = ffi.new("ib_rule_t*[1]")
        local rc

        rc = ffi.C.ib_rule_create(
            ib.ib_engine,
            ctx,
            rule.meta.source,
            rule.meta.line,
            rule.is_streaming(),
            prule)
        if rc ~= ffi.C.IB_OK then
            ib:logError("Failed to create rule.")
            return rc
        end

        -- For actions, set the magic actionflag.
        if rule.type == "actionsignature" then
            prule[0].flags = ffi.C.ib_set_flag(prule[0].flags, IB_RULE_FLAG_ACTION);
        end

        for _, action in ipairs(rule.data.actions) do
            local name, arg = action.name, action.argument

            if name == "logdata" then
              local expand = ffi.new("ib_var_expand_t*[1]")
              rc = ffi.C.ib_var_expand_acquire(
                  expand,
                  ffi.C.ib_engine_pool_main_get(ib.ib_engine),
                  arg,
                  #arg,
                  ffi.C.ib_engine_var_config_get(ib.ib_engine),
                  nil, nil
              )
              if rc ~= ffi.C.IB_OK then
                  ib:loggError("Failed to acquire rule data expand.")
              else
                  prule[0].meta.data = expand[0]
              end
            elseif name == "severity" then
                local severity = tonumber(arg)
                if severity > 255 then
                    ib:logError("Severity exceeds max value: %d", severity)
                elseif severity < 0 then
                    ib:logError("Severity is less than 0: %d", severity)
                else
                    prule[0].meta.severity = severity
                end
            elseif name == "confidence" then
                local confidence = tonumber(arg)
                if confidence > 255 then
                    ib:logError("Confidence exceeds max value: %d", confidence)
                elseif confidence < 0 then
                    ib:logError("Confidence is less than 0: %d", severity)
                else
                    prule[0].meta.confidence = confidence
                end

            -- Handling of Actions
            else

                -- Detect inverted actions (actions starting with !)
                local is_inverted = ffi.new("ib_rule_action_t")
                if string.sub(name, 1, 1) == '!' then
                    name = string.sub(name, 2)

                    -- fire false actions.
                    is_inverted = ffi.C.IB_RULE_ACTION_FALSE
                else
                    -- fire true actions.
                    is_inverted = ffi.C.IB_RULE_ACTION_TRUE
                end

                -- Create the action instance.
                local action_inst = ffi.new("ib_action_inst_t*[1]")
                rc = ffi.C.ib_action_inst_create(
                    ib.ib_engine,
                    name,
                    arg,
                    action_inst)
                if rc ~= ffi.C.IB_OK then
                    ib:logError(
                        "Failed to create action %s instance for rule.", name)
                    return rc
                end

                -- Add the action instance.
                rc = ffi.C.ib_rule_add_action(
                    ib.ib_engine,
                    prule[0],
                    action_inst[0],
                    is_inverted)
                if rc ~= ffi.C.IB_OK then
                    ib:logError("Failed to add action instance to rule.")
                    return rc
                end
            end
        end

        -- Set tags
        for tag, _ in pairs(rule.data.tags) do
            local tagcpy =
                ffi.C.ib_mpool_memdup(
                    ffi.C.ib_engine_pool_main_get(ib.ib_engine),
                    tag,
                    #tag + 1)
            ib:logDebug("Setting tag %s on rule.", tag)
            rc = ffi.C.ib_list_push(prule[0].meta.tags, tagcpy)
            if rc ~= ffi.C.IB_OK then
                ib:logError("Setting tag %s failed.", tag)
            end
        end

        -- Set message
        if rule.data.message then

            -- Set the message.
            local expand = ffi.new("ib_var_expand_t*[1]")
            rc = ffi.C.ib_var_expand_acquire(
                expand,
                ffi.C.ib_engine_pool_main_get(ib.ib_engine),
                rule.data.message,
                #rule.data.message,
                ffi.C.ib_engine_var_config_get(ib.ib_engine),
                nil, nil
            )
            if rc ~= ffi.C.IB_OK then
                ib:loggError("Failed to acquire rule msg expand.")
            else
                prule[0].meta.msg = expand[0]
            end

        end

        -- If this rule is a member of a chain.
        if i < #chain then
            rc = ffi.C.ib_rule_set_chain(ib.ib_engine, prule[0])
            if rc ~= ffi.C.IB_OK then
                ib:logError("Failed to setup chain rule.")
            end
        end

        if rule.data.has_predicate then
            -- Predicates do not have targets
            prule[0].flags = ffi.C.ib_set_flag(
                prule[0].flags,
                IB_RULE_FLAG_ACTION)
        else
            for _, field in ipairs(rule.data.fields) do
                add_fields(ib, rule, prule, field)
            end
        end

        -- Create operator instance.
        local opinst = ffi.new("void*[1]")
        local op_inst_create_stream_flags
        local op = ffi.new("ib_operator_t*[1]")

        -- Set the flag values.
        if rule.is_streaming() then
            op_inst_create_stream_flags = IB_OP_CAPABILITY_STREAM
        else
            op_inst_create_stream_flags = IB_OP_CAPABILITY_NON_STREAM
        end

        local opname

        -- Get the operator instance. If not defined, nop is used.
        if rule.data.op == nil or #rule.data.op == 0 then
            opname = "nop"
        elseif string.sub(rule.data.op, 1, 1) == '!' then
            opname = string.sub(rule.data.op, 2)
        else
            opname = rule.data.op
        end

        rc = ffi.C.ib_operator_lookup(
            ib.ib_engine,
            opname,
            ffi.cast("const ib_operator_t**", op))
        if rc ~= ffi.C.IB_OK then
            ib:logError("Could not locate operator %s", opname)
            return rc
        end

        -- C operator parameter copy. This is passed to the operater inst constructor and set in the rule.
        local cop_params = ffi.C.ib_mpool_memdup(
            ffi.C.ib_engine_pool_main_get(ib.ib_engine),
            tostring(rule.data.op_arg),
            #(tostring(rule.data.op_arg)) + 1)

        -- Create the operator.
        rc = ffi.C.ib_operator_inst_create(
            op[0],
            ctx,
            op_inst_create_stream_flags,
            cop_params,
            opinst)
        if rc ~= ffi.C.IB_OK then
            ib:logError("Failed to create operator instance for %s.", op)
            return rc
        end

        -- Set operator
        rc = ffi.C.ib_rule_set_operator(
            ib.ib_engine,
            prule[0],
            op[0],
            opinst[0])
        if rc ~= ffi.C.IB_OK then
            ib:logError("Failed to set rule operator.")
            return rc
        end

        -- Copy the parameters used to construct the operator instance into the rule for logging.
        ib:logInfo("Setting rule op inst params: %s", ffi.string(cop_params));
        rc = ffi.C.ib_rule_set_op_params(prule[0], cop_params);
        if rc ~= ffi.C.IB_OK then
            ib:logError("Failed to copy params %s.", ffi.string(cop_params))
            return rc
        end

        -- Invert the operator.
        if string.sub(rule.data.op, 1, 1) == '!' then
            rc = ffi.C.ib_rule_set_invert(prule[0], 1)
            if rc ~= ffi.C.IB_OK then
                ib:logError("Failed to invert operator %s.", op)
                return rc
            end
        end

        -- Find out of this is streaming or not, and treat that as an int.
        local is_streaming
        if rule.is_streaming() then
            is_streaming = 1
        else
            is_streaming = 0
        end

        -- If this rule is the first rule, it carries the id, rev, and phase
        -- for the chain of rules to follow. Set those values to the
        -- values in the last rule in the chain. Notice that rules
        -- that are in chains of length=1 this sets their id, rev, and phaes
        -- correctly.
        if i == 1 then
            -- Get last rule in the chain.
            local last_rule = db.db[chain[#chain].rule]

            -- Set id.
            ffi.C.ib_rule_set_id(ib.ib_engine, prule[0], last_rule.data.id)

            -- Set rev.
            prule[0].meta.revision = tonumber(last_rule.data.version) or 1

            -- Lookup and set phase.
            rc = ffi.C.ib_rule_set_phase(
                ib.ib_engine,
                prule[0],
                ffi.C.ib_rule_lookup_phase(last_rule.data.phase, is_streaming))
            if rc ~= ffi.C.IB_OK then
                ib:logError("Cannot set phase %s", last_rule.data.phase)
            end
        end

        rc = ffi.C.ib_rule_register(ib.ib_engine, ctx, prule[0])
        if rc ~= ffi.C.IB_OK then
            ib:logError("Failed to register rule.")
            return rc
        end

    end

    return ffi.C.IB_OK
end

--
-- Build and add all rules configured to the engine.
-- param[in] ib_ptr IronBee engine ib_engine_t*.
--
_M.build_rules = function(ib_ptr)
    local ib = ibapi.engineapi:new(ffi.cast("ib_engine_t*", ib_ptr))

    -- Get the main context. All rules are added to the main context.
    local mainctx = ffi.C.ib_context_main(ib_ptr)

    ib:logInfo("Validating rules...")
    local validator = Waggle:Validate()
    if type(validator) ~= 'string' then
        if validator:has_warnings() then
            for _, rec in ipairs(validator.warnings) do
                ib:logWarn("%s:%d Rule %s %d - %s",
                    rec.source,
                    rec.line,
                    rec.sig_id,
                    rec.sig_rev,
                    rec.msg)
            end
        else
            ib:logInfo("No warnings found")
        end

        if validator:has_errors() then
            for _, rec in ipairs(validator.warnings) do
                ib:logError("%s:%d Rule %s %d - %s",
                    rec.source,
                    rec.line,
                    rec.sig_id,
                    rec.sig_rev,
                    rec.msg)
            end
        else
            ib:logInfo("No errors found")
        end
    else
        ib:logInfo("Validation found no problems.")
    end

    local plan = Waggle:Plan()
    local db = Waggle.DEFAULT_RULE_DB
    for _, chain in ipairs(plan) do
        local rc = build_rule(ib, mainctx, chain, db)
        if rc ~= ffi.C.IB_OK then
            return rc
        end
    end

    -- We're done. Clear out the rules.
    Waggle:clear_rule_db()

    return ffi.C.IB_OK
end

return _M
