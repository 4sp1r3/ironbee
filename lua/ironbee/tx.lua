-- =========================================================================
-- Licensed to Qualys, Inc. (QUALYS) under one or more
-- contributor license agreements.  See the NOTICE file distributed with
-- this work for additional information regarding copyright ownership.
-- QUALYS licenses this file to You under the Apache License, Version 2.0
-- (the "License"); you may not use this file except in compliance with
-- the License.  You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--
-- =========================================================================

-------------------------------------------------------------------
-- IronBee - Transaction API.
--
-- @module ironbee.tx
--
-- @copyright Qualys, Inc., 2010-2015
-- @license Apache License, Version 2.0
--
-- @author Sam Baskinger <sbaskinger@qualys.com>
-------------------------------------------------------------------

local engine = require('ironbee/engine')
local ffi = require('ffi')
local ibutil = require('ironbee/util')
local ibcutil = require('ibcutil')
local ib_logevent = require('ironbee/logevent')

ffi.cdef [[
/* Transaction Flags */
enum ib_lua_transaction_flags {
    IB_TX_FNONE               = (0),
    IB_TX_FHTTP09             = (1 <<  0),
    IB_TX_FPIPELINED          = (1 <<  1),

    IB_TX_FREQ_STARTED        = (1 <<  3),
    IB_TX_FREQ_LINE           = (1 <<  4),
    IB_TX_FREQ_HEADER         = (1 <<  5),
    IB_TX_FREQ_BODY           = (1 <<  6),
    IB_TX_FREQ_TRAILER        = (1 <<  7),
    IB_TX_FREQ_FINISHED       = (1 <<  8),
    IB_TX_FREQ_HAS_DATA       = (1 <<  9),

    IB_TX_FRES_STARTED        = (1 << 10),
    IB_TX_FRES_LINE           = (1 << 11),
    IB_TX_FRES_HEADER         = (1 << 12),
    IB_TX_FRES_BODY           = (1 << 13),
    IB_TX_FRES_TRAILER        = (1 << 14),
    IB_TX_FRES_FINISHED       = (1 << 15),
    IB_TX_FRES_HAS_DATA       = (1 << 16),

    IB_TX_FLOGGING            = (1 << 17),
    IB_TX_FPOSTPROCESS        = (1 << 18),

    IB_TX_FERROR              = (1 << 19),
    IB_TX_FSUSPICIOUS         = (1 << 20),

    IB_TX_FINSPECT_REQURI     = (1 << 22),
    IB_TX_FINSPECT_REQPARAMS  = (1 << 23),
    IB_TX_FINSPECT_REQHDR     = (1 << 24),
    IB_TX_FINSPECT_REQBODY    = (1 << 25),
    IB_TX_FINSPECT_RESHDR     = (1 << 26),
    IB_TX_FINSPECT_RESBODY    = (1 << 27),

    IB_TX_FBLOCKING_MODE      = (1 << 28),
    IB_TX_FBLOCK_ADVISORY     = (1 << 29),
    IB_TX_FBLOCK_PHASE        = (1 << 30),
    IB_TX_FBLOCK_IMMEDIATE    = (1 << 31),
    IB_TX_FALLOW_PHASE        = (1 << 32),
    IB_TX_FALLOW_REQUEST      = (1 << 33),
    IB_TX_FALLOW_ALL          = (1 << 34)
};
]]

-- Event Type Map used by addEvent.
-- Default values is 'unknown'
local eventTypeMap = {
    observation = ffi.C.IB_LEVENT_TYPE_OBSERVATION,
    alert       = ffi.C.IB_LEVENT_TYPE_ALERT,
    unknown     = ffi.C.IB_LEVENT_TYPE_UNKNOWN
}
setmetatable(eventTypeMap, { __index = ibutil.returnUnknown })

-- Action Map used by addEvent.
-- Default values is 'unknown'
local actionMap = {
    allow   = ffi.C.IB_LEVENT_ACTION_ALLOW,
    block   = ffi.C.IB_LEVENT_ACTION_BLOCK,
    ignore  = ffi.C.IB_LEVENT_ACTION_IGNORE,
    log     = ffi.C.IB_LEVENT_ACTION_LOG,
    unknown = ffi.C.IB_LEVENT_ACTION_UNKNOWN
}
setmetatable(actionMap, { __index = ibutil.returnUnknown })


local M = {}
M.__index = M

setmetatable(M, engine)

M.new = function(self, ib_engine, ib_tx)
    local o = engine:new(ib_engine)

    -- Store raw C values.
    o.ib_tx = ib_tx

    return setmetatable(o, self)
end

-- Set transaction flags
M.setFlags = function(self, flags)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local tx_flags = ffi.cast("ib_flags_t", flags)

    return ffi.C.ib_tx_flags_set(tx, tx_flags)
end

-- Unset transaction flags
M.unsetFlags = function(self, flags)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local tx_flags = ffi.cast("ib_flags_t", flags)

    return ffi.C.ib_tx_flags_unset(tx, tx_flags)
end

-- Return a list of all the fields currently defined.
M.getFieldList = function(self)
    local fields = { }

    local ib_list = ffi.new("ib_list_t*[1]")
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    ffi.C.ib_list_create(ib_list, tx.mm)
    ffi.C.ib_var_store_export(tx.var_store, ib_list[0])

    ibutil.each_list_node(ib_list[0], function(field)
        fields[#fields+1] = ffi.string(field.name, field.nlen)

        ib_list_node = ffi.C.ib_list_node_next(ib_list_node)
    end)

    return fields
end

-- Add a string, number, or table to the transaction data.
-- If value is a string or a number, it is appended to the end of the
-- list of values available through the data.
-- If the value is a table, and the table exists in the data,
-- then the values are appended to that table. Otherwise, a new
-- table is created.
M.add = function(self, name, value)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local target = self:getVarTarget(name)
    local rc

    if value == nil then
        -- nop.
    elseif type(value) == 'string' or type(value) == 'number' then
        local ib_field = ffi.new("ib_field_t*[1]")
        if type(value) == 'string' then
            rc = ffi.C.ib_field_create_bytestr_alias(
              ib_field,
              tx.mm,
              ffi.cast("char*", ""), 0,
              ffi.C.ib_mm_memdup(tx.mm, ffi.cast("char*", value), #value),
              #value
            )
        elseif type(value) == 'number' then
            if value == math.floor(value) then
                local fieldValue_p = ffi.new("ib_num_t[1]", value)
                rc = ffi.C.ib_field_create(
                    ib_field,
                    tx.mm,
                    ffi.cast("char*", ""), 0,
                    ffi.C.IB_FTYPE_NUM,
                    fieldValue_p
                )
            else
                -- Set a float.
                local fieldValue_p = ffi.new("ib_float_t[1]")
                ibcutil.to_ib_float(fieldValue_p, value)
                rc = ffi.C.ib_field_create(
                    ib_field,
                    tx.mm,
                    ffi.cast("char*", ""), 0,
                    ffi.C.IB_FTYPE_FLOAT,
                    fieldValue_p)
                if rc ~= ffi.C.IB_OK then
                    self:logError("Cannot create float field.")
                end
            end
        end
        if rc ~= ffi.C.IB_OK then
            self:logError("Could not create field for %s", name)
        end
        rc = ffi.C.ib_var_target_set(target, tx.mm, tx.var_store, ib_field[0])
        if rc ~= ffi.C.IB_OK then
            self:logError("Could not set source for %s", name)
        end
    elseif type(value) == 'table' then
        local source = ffi.new("ib_var_source_t*[1]")
        local ib_field = ffi.new("ib_field_t*[1]")

        rc = ffi.C.ib_var_source_acquire(
            source,
            tx.mm,
            ffi.C.ib_engine_var_config_get(tx.ib),
            name,
            #name)
        if rc ~= ffi.C.IB_OK then
            self:logError("Failed to allocate var source.")
        end

        rc = ffi.C.ib_var_source_get(source[0], ib_field, tx.var_store)
        if rc == ffi.C.IB_ENOENT or
           (rc == ffi.C.IB_OK and ib_field[0].type ~= ffi.C.IB_FTYPE_LIST) then
            rc = ffi.C.ib_var_source_initialize(
                source[0],
                ib_field,
                tx.var_store,
                ffi.C.IB_FTYPE_LIST
            )
        end
        if rc ~= ffi.C.IB_OK then
            self:logError("Could not get table field for %s", name)
        end
        for k,v in ipairs(value) do
            self:appendToList(name, v[1], v[2])
        end
    else
        self:logError("Unsupported type %s", type(value))
    end
end

M.set = function(self, name, value)

    local ib_list = self:getVarFields(name)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local ib_target = self:getVarTarget(name)
    local ib_field = ffi.new("ib_field_t*[1]")

    if ib_list == nil or ffi.C.ib_list_elements(ib_list) == 0 then
        -- If ib_list is empty, then it doesn't exist and we call add(...).
        -- It is not an error, if value==nil, to pass value to add.
        -- Adding nil is a nop.
        self:add(name, value)
    elseif value == nil then
        -- Delete values when setting a name to nil.
        self:setVarField(name, nil)
    elseif type(value) == 'string' then
        rc = ffi.C.ib_field_create_bytestr_alias(
          ib_field,
          tx.mm,
          ffi.cast("char*", ""), 0,
          ffi.C.ib_mm_memdup(tx.mm, ffi.cast("char*", value), #value),
          #value
        )
        if rc ~= ffi.C.IB_OK then
            self:logError("Failed to allocate string field.")
        end
        ffi.C.ib_var_target_remove_and_set(
            ib_target,
            tx.mm,
            tx.var_store,
            ib_field[0]
        )
    elseif type(value) == 'number' then
        if value == math.floor(value) then
            -- Set a number.
            local num = ffi.new("ib_num_t[1]", value)
            rc = ffi.C.ib_field_create(
                ib_field,
                tx.mm,
                ffi.cast("char*", ""), 0,
                ffi.C.IB_FTYPE_NUM,
                num)
            if rc ~= ffi.C.IB_OK then
                self:logError("Cannot create num field.")
            end
        else
            -- Set a float.
            local flt = ffi.new("ib_float_t[1]")
            ibcutil.to_ib_float(flt, value)
            rc = ffi.C.ib_field_create(
                ib_field,
                tx.mm,
                ffi.cast("char*", ""), 0,
                ffi.C.IB_FTYPE_FLOAT,
                flt)
            if rc ~= ffi.C.IB_OK then
                self:logError("Cannot create num field.")
            end
        end
        ffi.C.ib_var_target_remove_and_set(
            ib_target,
            tx.mm,
            tx.var_store,
            ib_field[0]
        )
    elseif type(value) == 'table' then
        rc = ffi.C.ib_var_target_remove(ib_target, nil, ffi.C.IB_MM_NULL, tx.var_store)
        if rc ~= ffi.C.IB_OK then
            self:logError("Failed to remove target %s", name)
        end
        self:add(name, value)
    else
        self:logError("Unsupported type %s", type(value))
    end
end


-- Get list of values from the transaction's data instance.
--
-- If that parameter points to a string, a string is returned.
-- If name points to a number, a number is returned.
-- If name points to a list of name-value pairs a table is returned
--    where
--
-- @param[in] name The target to fetch all values for and convert into Lua types.
--
-- @returns A list of values.
M.get = function(self, name)
    local rc
    local ib_tx     = ffi.cast("ib_tx_t *", self.ib_tx)
    local ib_list   = ffi.new("ib_list_t*[1]")
    local ib_field  = ffi.new("ib_field_t*[1]")
    local ib_target = self:getVarTarget(name)
    local ib_source = ffi.C.ib_var_target_source(ib_target)

    -- Get the source so as to check its type and choose a return format.
    -- If ib_field is a list, return a list.
    -- If ib_field is not a list, return a raw value.
    rc = ffi.C.ib_var_source_get(
        ib_source,
        ib_field,
        ib_tx.var_store)
    if rc == ffi.C.IB_ENOENT then
        return nil
    elseif rc ~= ffi.C.IB_OK then
        error("Failed to get source of target "..name)
    end

    -- Get all the entries.
    rc = ffi.C.ib_var_target_get_const(
        ib_target,
        ffi.cast("const ib_list_t**", ib_list),
        ib_tx.mm,
        ib_tx.var_store)
    if rc == ffi.C.IB_ENOENT then
        return nil
    elseif rc ~= ffi.C.IB_OK then
        self:logError(
            "Could not get value for %s: %s",
            name,
            ffi.string(ffi.C.ib_status_to_string(rc)))
    end

    -- If the field is not a list type, just return whatever
    -- fetching the target returns.
    if ib_field[0].type ~= ffi.C.IB_FTYPE_LIST then

        -- NOTE: We use ib_list here, not ib_field.
        --       This is to correctly detect that a value "A:a" does
        --       not exist if a value A does exist as a scalar.
        --       If we used ib_field to return the value instead of ib_list
        --       we would incorrectly find "A" and return it when the user asked for
        --       the non-existant value "A:a"
        local node = ffi.C.ib_list_first(ib_list[0])
        local data = ffi.C.ib_list_node_data(node)
        return self:fieldToLua(ffi.cast("ib_field_t*", data))
    end

    local v = {}
    -- ib_field[0] is a list, so we return all values in a table format.
    if ib_list[0] ~= nil then
        ibutil.each_list_node(
            ib_list[0],
            function(f)
                table.insert(
                    v,
                    {
                        ffi.string(f.name, f.nlen),
                        self:fieldToLua(f)
                    }
                )
            end
        )
    end

    return v
end

-- Given a field name, this will return a list of the field names
-- contained in it. If the requested field is a string or an integer, then
-- a single element list containing name is returned.
M.getNames = function(self, name)
    local ib_list = self:getVarFields(name)
    local t = {}

    ibutil.each_list_node(
        ib_list,
        function(ib_field)
            -- To speed things up, we handle a list directly
            if ib_field.type == ffi.C.IB_FTYPE_LIST then
                local value = ffi.new("ib_list_t*[1]")
                ffi.C.ib_field_value(ib_field, value)

                ibutil.each_list_node(value[0], function(data)
                    table.insert(t, ffi.string(data.name, data.nlen))
                end)
            else
                table.insert(t, ffi.string(ib_field.name, ib_field.nlen))
            end
        end)

    return t
end

-- Given a field name, this will return a list of the values that are
-- contained in it. If the requeted field is a string or an integer,
-- then a single element list containing that value is returned.
M.getValues = function(self, name)

    local ib_list = self:getVarFields(name)
    local t = {}
    ibutil.each_list_node(
        ib_list,
        function(ib_field)
            -- To speed things up, we handle a list directly
            if ib_field.type == ffi.C.IB_FTYPE_LIST then
                local value =  ffi.new("ib_list_t*[1]")
                ffi.C.ib_field_value(ib_field, value)

                ibutil.each_list_node(value[0], function(data)
                    table.insert(t, self:fieldToLua(data))
                end)
            else
                table.insert(t, self:fieldToLua(ib_field))
            end
        end)

    return t
end

--
-- Call function func on each event in the current transaction.
--
M.forEachEvent = function(self, func)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local list = ffi.new("ib_list_t*[1]")
    ffi.C.ib_logevent_get_all(tx, list)

    ibutil.each_list_node(
        list[0],
        function(event)
            func(ib_logevent:new(event))
        end ,
        "ib_logevent_t*")
end

--
-- Fetch the last event from the transaction.
--
-- @returns
-- - Nil if there are no events.
-- - A ironbee/logevent object.
M.lastEvent = function(self)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local e  = ffi.new("ib_logevent_t *[1]", nil)

    ffi.C.ib_logevent_get_last(tx, e)

    return ib_logevent:new(e[0])
end

-- Private event next function. This is used to build iteration
-- functions like all_events and events.
--
-- The table t must have a function skip_node defined that
-- takes a list node and returns true if it is should be skipped.
--
-- The table t must have a c pointer to ib_tx_t to retrieve the initial
-- list of events.
--
-- Skipped nodes do not increment the index.
local event_next_fn = function(t, idx)
    -- Iterate 1 step.
    if idx == nil then
        local list = ffi.new("ib_list_t*[1]")
        ffi.C.ib_logevent_get_all(t.tx, list)

        if (list[0] == nil) then
            return nil, nil
        end

        t.i = 0
        t.node = ffi.cast("ib_list_node_t*", ffi.C.ib_list_first(list[0]))
    else
        t.i = idx + 1
        t.node = ffi.C.ib_list_node_next(t.node)
    end

    -- Conditionally skip nodes in the list we don't want to iterate over.
    while t.node ~= nil and t.skip_node(t.node) do
        t.node = ffi.C.ib_list_node_next(t.node)
    end

    -- End of list.
    if t.node == nil then
        return nil, nil
    end

    -- Get event and convert it to lua.
    local event =
        ib_logevent:new(ffi.cast("ib_logevent_t*",
            ffi.C.ib_list_node_data(t.node)))

    -- Return.
    return t.i, event
end

-- Returns next function, table, and nil.
M.events = function(self)
    return event_next_fn,
           {
               skip_node = function(node)
                   local event = ffi.cast("ib_logevent_t*",
                       ffi.C.ib_list_node_data(node))
                   -- Skip any suppressed events.
                   return event.suppress ~= ffi.C.IB_LEVENT_SUPPRESS_NONE
               end,
               tx = ffi.cast("ib_tx_t *", self.ib_tx)
           },
           nil
end

M.all_events = function(self)
    return event_next_fn,
           {
             skip_node = function()
                 return false
             end,
             tx = ffi.cast("ib_tx_t *", self.ib_tx)
           },
           nil
end

-- Append a value to the end of the name list. This may be a string
-- or a number. This is used by ib_obj.add to append to a list.
M.appendToList = function(self, listName, fieldName, fieldValue)

    local field = ffi.new("ib_field_t*[1]")
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)

    if type(fieldValue) == 'string' then
        -- Create the field
        ffi.C.ib_field_create(field,
                                 tx.mm,
                                 ffi.cast("char*", fieldName),
                                 #fieldName,
                                 ffi.C.IB_FTYPE_NULSTR,
                                 ffi.cast("char*", fieldValue))

    elseif type(fieldValue) == 'number' then
        if fieldValue == math.floor(fieldValue) then
            local fieldValue_p = ffi.new("ib_num_t[1]", fieldValue)

            ffi.C.ib_field_create(field,
                                  tx.mm,
                                  ffi.cast("char*", fieldName),
                                  #fieldName,
                                  ffi.C.IB_FTYPE_NUM,
                                  fieldValue_p)
        else
            local fieldValue_p = ffi.new("ib_float_t[1]")
            ibcutil.to_ib_float(fiedValue_p, fieldValue)

            ffi.C.ib_field_create(field,
                                  tx.mm,
                                  ffi.cast("char*", fieldName),
                                  #fieldName,
                                  ffi.C.IB_FTYPE_FLOAT,
                                  fieldValue_p)
        end
    else
        return
    end

    local source = ffi.new("ib_var_source_t*[1]")
    local ib_field = ffi.new("ib_field_t*[1]")

    rc = ffi.C.ib_var_source_acquire(
        source,
        tx.mm,
        ffi.C.ib_engine_var_config_get(tx.ib),
        listName,
        #listName)
    if rc ~= ffi.C.IB_OK then
        error("Failed to allocate var source.")
    end

    rc = ffi.C.ib_var_source_get(source[0], ib_field, tx.var_store)
    if rc ~= ffi.C.IB_OK then
        error(string.format("Could not get table field for %s", name))
    end

    -- Append the field that is last in this.
    ffi.C.ib_field_list_add(ib_field[0], field[0])
end

-- Add an event.
-- The msg argument is typically a string that is the message to log,
-- followed by a table of options.
--
-- If msg is a table, however, then options is ignored and instead
-- msg is processed as if it were the options argument. Think of this
-- as the argument msg being optional.
--
-- If msg is omitted, then options should contain a key 'msg' that
-- is the message to log.
--
-- The options argument should also specify the following (or they will
-- default to UNKNOWN):
--
-- recommended_action - The recommended action.
--     - block
--     - ignore
--     - log
--     - unknown (default)
-- action - The action to take. Values are the same as recommended_action.
-- type - The rule type that was matched.
--     - observation
--     - unknown (default)
-- confidence - An integer. The default is 0.
-- severity - An integer. The default is 0.
-- msg - If msg is not given, then this should be the alert message.
-- tags - List (table) of tag strings: { 'tag1', 'tag2', ... }
--
M.addEvent = function(self, msg, options)

    local message

    -- If msg is a table, then options are ignored.
    if type(msg) == 'table' then
        options = msg
        message = ffi.cast("char*", msg['msg'] or '-')
    else
        message = ffi.cast("char*", msg)
    end

    if options == nil then
        options = {}
    end

    local event = ffi.new("ib_logevent_t*[1]")
    local rulename = ffi.cast("char*", options['rulename'] or 'anonymous')

    -- Map options
    local rec_action      = actionMap[options.recommended_action]
    local event_type      = eventTypeMap[options.type]
    local confidence      = options.confidence or 0
    local severity        = options.severity or 0

    local tx = ffi.cast("ib_tx_t *", self.ib_tx)

    ffi.C.ib_logevent_create(event,
                             tx.mm,
                             rulename,
                             event_type,
                             rec_action,
                             confidence,
                             severity,
                             message
                            )

    -- Add tags
    if options.tags ~= nil then
        if type(options.tags) == 'table' then
            for k,v in ipairs(options.tags) do
                ffi.C.ib_logevent_tag_add(event[0], v)
            end
        end
    end

    ffi.C.ib_logevent_add(tx, event[0])
end

M.getVarSource = function(self, name)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)

    local ib_source = ffi.new("ib_var_source_t *[1]")

    local rc

    rc = ffi.C.ib_var_source_acquire(
        ib_source,
        tx.mm,
        ffi.C.ib_engine_var_config_get(tx.ib),
        ffi.cast("char *", name),
        #name
    )
    if rc ~= ffi.C.IB_OK then
        self:logError("Could not acquire source for %s", name)
    end

    return ib_source[0]
end

M.getVarTarget = function(self, name)
    local ib_tx     = ffi.cast("ib_tx_t *", self.ib_tx)
    local ib_target = ffi.new("ib_var_target_t*[1]")
    local rc

    rc = ffi.C.ib_var_target_acquire_from_string(
        ib_target,
        ib_tx.mm,
        ffi.C.ib_engine_var_config_get(ib_tx.ib),
        name,
        #name
    )
    if rc ~= ffi.C.IB_OK then
        self:logError("Error at in %s", name)
    end

    return ib_target[0]
end

-- Return an ib_list_t* of ib_field_t* to the field named and stored in the DPI.
--
-- Nil is retrned if no list is available.
-- This is used to quickly pull named fields for setting or getting values.
--
-- @param[in] name The name of a target to get all values for.
--
-- @returns A list of values or nil if there are no values.
M.getVarFields = function(self, name)
    local ib_tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local ib_target = self:getVarTarget(name)
    local ib_list = ffi.new("ib_list_t*[1]")
    local rc

    rc = ffi.C.ib_var_target_get_const(
        ib_target,
        ffi.cast("const ib_list_t**", ib_list),
        ib_tx.mm,
        ib_tx.var_store)
    if rc == ffi.C.IB_ENOENT then
        return nil
    elseif rc ~= ffi.C.IB_OK then
        self:logError(
            "Could not get value for %s: %s",
            name,
            ffi.string(ffi.C.ib_status_to_string(rc)))
    end

    return ib_list[0]
end

M.setVarField = function(self, name, ib_field)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local ib_target = self:getVarTarget(name)
    local rc

    rc = ffi.C.ib_var_target_set(ib_target, tx.mm, tx.var_store, ib_field[0])
    if rc ~= ffi.C.IB_OK then
        self:logError("Could not set value for %s", name)
    end
end

M.getConnectionId = function(self)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)

    return ffi.string(tx.conn.id, 36)
end

M.getTransactionId = function(self)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)

    return ffi.string(tx.id, 36)
end

-- Block the transaction.
--
-- @returns
-- - IB_OK on success (including if tx already blocked).
-- - IB_DECLINED if blocking is disabled or block handler returns IB_DECLINED.
-- - IB_ENOTIMPL if the server does not support the desired blocking method.
-- - Other if server, handler, or callback reports error.
function M:block()
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    return ffi.C.ib_tx_block(tx)
end

-- Allow blocking of the transaction.
function M:enableBlocking()
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    ffi.C.ib_tx_enable_blocking(tx)
end

-- Disable blocking of the transaction.
function M:disableBlocking()
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    ffi.C.ib_tx_disable_blocking(tx)
end

-- Is the transaction blockable.
--
-- @returns
-- - True of blocking is enabled.
-- - False otherwise.
function M:isBlockingEnabled()
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    return ffi.C.ib_tx_is_blocking_enabled(tx)
end

-- Is the transaction blocked?
--
-- @returns
-- - True if the transaction is blocked.
-- - False otherwise.
function M:isBlocked()
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    return ffi.C.ib_tx_is_blocked(tx)
end

-- Is the transaction allowed?
--
-- @returns
-- - True if the transaction is allowed.
-- - False otherwise.
function M:isAllowed()
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    return ffi.C.ib_tx_is_allowed(tx)
end

function M:setRequestHeader(name, val)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local rc = ffi.C.ib_tx_server_header(
        tx,
        ffi.C.IB_SERVER_REQUEST,
        ffi.C.IB_HDR_SET,
        name,
        string.len(name),
        val,
        string.len(val))
    if rc ~= ffi.C.IB_OK then
        self:logError("Failed to set request header %s.", name)
    end
end

function M:addRequestHeader(name, val)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local rc = ffi.C.ib_tx_server_header(
        tx,
        ffi.C.IB_SERVER_REQUEST,
        ffi.C.IB_HDR_ADD,
        name,
        string.len(name),
        val,
        string.len(val))
    if rc ~= ffi.C.IB_OK then
        self:logError("Failed to add request header %s.", name)
    end
end

function M:delRequestHeader(name, val)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local rc = ffi.C.ib_tx_server_header(
        tx,
        ffi.C.IB_SERVER_REQUEST,
        ffi.C.IB_HDR_UNSET,
        name,
        string.len(name),
        val,
        string.len(val))
    if rc ~= ffi.C.IB_OK then
        self:logError("Failed to unset request header %s.", name)
    end
end

function M:setResponseHeader(name, val)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local rc = ffi.C.ib_tx_server_header(
        tx,
        ffi.C.IB_SERVER_RESPONSE,
        ffi.C.IB_HDR_SET,
        name,
        string.len(name),
        val,
        string.len(val))
    if rc ~= ffi.C.IB_OK then
        self:logError("Failed to set response header %s.", name)
    end
end

function M:addResponseHeader(name, val)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local rc = ffi.C.ib_tx_server_header(
        tx,
        ffi.C.IB_SERVER_RESPONSE,
        ffi.C.IB_HDR_ADD,
        name,
        string.len(name),
        val,
        string.len(val))
    if rc ~= ffi.C.IB_OK then
        self:logError("Failed to add response header %s.", name)
    end
end

function M:delResponseHeader(name, val)
    local tx = ffi.cast("ib_tx_t *", self.ib_tx)
    local rc = ffi.C.ib_tx_server_header(
        tx,
        ffi.C.IB_SERVER_RESPONSE,
        ffi.C.IB_HDR_UNSET,
        name,
        string.len(name),
        val,
        string.len(val))
    if rc ~= ffi.C.IB_OK then
        self:logError("Failed to unset response header %s.", name)
    end
end

return M
