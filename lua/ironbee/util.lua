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

local ffi = require('ffi')

local _M = {}
_M.__index = _M
_M._COPYRIGHT = "Copyright (C) 2010-2014 Qualys, Inc."
_M._DESCRIPTION = "IronBee Lua Utility Methods"
_M._VERSION = "1.0"


-- This is a replacement for __index in a table's metatable.
-- It will, when receiving an index it does not have an entry for,
-- return the 'unknown' entry in the table.
_M.returnUnknown = function(self, key)
    if key == 'unknown' then
        return nil
    else
        return self['unknown']
    end
end

-- Iterate over the ib_list (of type ib_list_t *) calling the
-- function func on each ib_field_t* contained in the elements of ib_list.
-- The resulting list data is passed to the callback function
-- as a "ib_field_t*" or if cast_type is specified, as that type.
_M.each_list_node = function(ib_list, func, cast_type)
    local ib_list_node = ffi.cast("ib_list_node_t*",
                                  ffi.C.ib_list_first(ib_list))
    if cast_type == nil then
      cast_type = "ib_field_t*"
    end

    while ib_list_node ~= nil do
        -- Callback
        func(ffi.cast(cast_type, ffi.C.ib_list_node_data(ib_list_node)))

        -- Next
        ib_list_node = ffi.C.ib_list_node_next(ib_list_node)
    end
end

return _M
