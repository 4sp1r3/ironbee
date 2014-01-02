
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
--
-- LogEvent - A security event generated by IronBee rules.
--
-- Author: Sam Baskinger <sbaskinger@qualys.com>
--
-- =========================================================================

local ibutil = require('ironbee/util')
local ffi = require('ffi')
local tx = require('ironbee/tx')

local _M = {}
_M.__index = _M
_M._COPYRIGHT = "Copyright (C) 2010-2014 Qualys, Inc."
_M._DESCRIPTION = "IronBee Lua Rules"
_M._VERSION = "1.0"

setmetatable(_M, tx)

_M.new = function(self, ib_rule_exec, ib_engine, ib_tx)
    local o = tx:new(ib_engine, ib_tx)

    -- Store raw C values.
    o.ib_rule_exec = ffi.cast("const ib_rule_exec_t*", ib_rule_exec)

    return setmetatable(o, self)
end

-- The private logging function. This function should only be called
-- by self:logError(...) or self:logDebug(...) or the file and line
-- number will not be accurage because the call stack will be at an
-- unexpected depth.
_M.log = function(self, level, prefix, msg, ...)
    local debug_table = debug.getinfo(3, "Sl")
    local file = debug_table.short_src
    local line = debug_table.currentline

    -- Msg must not be nil.
    if msg == nil then msg = "(nil)" end

    if type(msg) ~= 'string' then msg = tostring(msg) end

    -- If we have more arguments, format msg with them.
    if ... ~= nil then msg = string.format(msg, ...) end

    -- Prepend prefix.
    msg = prefix .. " " .. msg

    -- Log the string.
    ffi.C.ib_rule_log_exec(level, self.ib_rule_exec, file, line, msg);
end

-- Log an error.
_M.logError = function(self, msg, ...)
    self:log(ffi.C.IB_RULE_DLOG_ERROR, "LuaAPI - [ERROR]", msg, ...)
end

-- Log a warning.
_M.logWarn = function(self, msg, ...)
    -- Note: Extra space after "INFO " is for text alignment.
    -- It should be there.
    self:log(ffi.C.IB_RULE_DLOG_WARNING, "LuaAPI - [WARN ]", msg, ...)
end

-- Log an info message.
_M.logInfo = function(self, msg, ...)
    -- Note: Extra space after "INFO " is for text alignment.
    -- It should be there.
    self:log(ffi.C.IB_RULE_DLOG_INFO, "LuaAPI - [INFO ]", msg, ...)
end

-- Log debug information at level 3.
_M.logDebug = function(self, msg, ...)
    self:log(ffi.C.IB_RULE_DLOG_DEBUG, "LuaAPI - [DEBUG]", msg, ...)
end

-- ###########################################################################

return _M
