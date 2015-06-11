--[[
LuCI - Lua Configuration Interface

Copyright 2008 Steven Barth <steven@midlink.org>
Copyright 2008 Jo-Philipp Wich <xm@leipzig.freifunk.net>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id: openvpn.lua 9507 2012-11-26 12:53:43Z jow $
]]--

module("luci.controller.openvpn", package.seeall)

function index()
	local fs = luci.fs or nixio.fs
	if not fs.access("/etc/config/openvpn") then
		return
	end

	local users = { "admin", "support", "user" }

	for k, user in pairs(users) do
		entry( {user, "services", "openvpn"}, cbi("openvpn"), _("OpenVPN") )
		entry( {user, "services", "openvpn", "basic"},    cbi("openvpn-basic"),    nil ).leaf = true
		entry( {user, "services", "openvpn", "advanced"}, cbi("openvpn-advanced"), nil ).leaf = true
	end
end
