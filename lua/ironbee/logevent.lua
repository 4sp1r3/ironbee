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
require('ironbee-ffi-h')

local _M = {}
_M.__index = _M
_M._COPYRIGHT = "Copyright (C) 2010-2014 Qualys, Inc."
_M._DESCRIPTION = "IronBee Lua Log Event"
_M._VERSION = "1.0"

_M.new = function(self, event)
    local o = { raw = event }
    return setmetatable(o, self)
end

-- String mapping table.
_M.suppressMap = {
    none           = tonumber(ffi.C.IB_LEVENT_SUPPRESS_NONE),
    false_positive = tonumber(ffi.C.IB_LEVENT_SUPPRESS_FPOS),
    replaced       = tonumber(ffi.C.IB_LEVENT_SUPPRESS_REPLACED),
    incomplete     = tonumber(ffi.C.IB_LEVENT_SUPPRESS_INC),
    partial        = tonumber(ffi.C.IB_LEVENT_SUPPRESS_INC),
    other          = tonumber(ffi.C.IB_LEVENT_SUPPRESS_OTHER)
}
-- Build reverse map.
_M.suppressRmap = {}
for k,v in pairs(_M.suppressMap) do
    _M.suppressRmap[v] = k
end

_M.typeMap = {
    unknown        = tonumber(ffi.C.IB_LEVENT_TYPE_UNKNOWN),
    observation    = tonumber(ffi.C.IB_LEVENT_TYPE_OBSERVATION),
    alert          = tonumber(ffi.C.IB_LEVENT_TYPE_ALERT),
}
-- Build reverse map.
_M.typeRmap = {}
for k,v in pairs(_M.typeMap) do
    _M.typeRmap[v] = k
end

_M.getSeverity = function(self)
    return self.raw.severity
end
_M.getConfidence = function(self)
    return self.raw.confidence
end
_M.getAction = function(self)
    return self.raw.action
end
_M.getRuleId = function(self)
    return ffi.string(self.raw.rule_id)
end
_M.getMsg = function(self)
    return ffi.string(self.raw.msg)
end
_M.getSuppress = function(self)
    return _M.suppressRmap[tonumber(self.raw.suppress)]
end
-- On an event object set the suppression value using a number or name.
-- value - may be none, false_positive, replaced, incomplete, partial or other.
--         The value of none indicates that there is no suppression of the event.
_M.setSuppress = function(self, value)
    if type(value) == "number" then
        self.raw.suppress = value
    else
        self.raw.suppress = _M.suppressMap[string.lower(value)] or 0
    end
end

_M.getType = function(self)
    return _M.typeRmap[tonumber(self.raw.type)]
end
_M.setType = function(self, value)
    if type(value) == "number" then
        self.raw.type = value
    else
        self.raw.type = _M.typeMap[string.lower(value)] or 0
    end
end
_M.forEachField = function(self, func)
    if self.raw.fields ~= nil then
        ibutil.each_list_node(
            self.raw.fields,
            function(charstar)
                func(ffi.string(charstar))
            end,
            "char*")
    end
end
_M.forEachTag = function(self, func)
    if self.raw.tags ~= nil then
        ibutil.each_list_node(
            self.raw.tags,
            function(charstar)
                func(ffi.string(charstar))
            end,
            "char*")
    end
end

-- Iteration function used by e:tags.
-- Given a table with an element "node" of type ib_list_node_t*
-- this will iterate across the nodes extracting the
-- ib_list_node_data as a string.
--
-- This is used by e:tags() and e:fields().
local next_string_fn = function(t, idx)
    local data

    if t.node == nil then
        return nil, nil
    else
        data = ffi.C.ib_list_node_data(t.node)
        data = ffi.string(data)
    end

    t.node = ffi.cast("ib_list_node_t*", ffi.C.ib_list_node_next(t.node))

    return idx + 1, data
end

_M.tags = function(self)
    local t = {
        node = ffi.cast("ib_list_node_t*", ffi.C.ib_list_first(self.raw.tags))
    }

    -- return function, table, and index before first.
    return next_string_fn, t, 0
end

_M.fields = function()
    local t = {
        node = ffi.cast("ib_list_node_t*", ffi.C.ib_list_first(self.raw.fields))
    }

    -- return function, table, and index before first.
    return next_string_fn, t, 0
end

return _M
