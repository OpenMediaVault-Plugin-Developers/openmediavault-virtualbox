<?php
/**
 * Copyright (C) 2010-2012 Ian Moore <imooreyahoo@gmail.com>
 * Copyright (C)      2013 OpenMediaVault Plugin Developers
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file. If not, see <http://www.gnu.org/licenses/>.
 */

require "openmediavault/rpc.inc";

use SplFileInfo as FileInfo;
use OMVRpc;

class phpVBoxConfig
{
    // Static config items
    public $location = "http://127.0.0.1:18083/";
    public $language = "en";
    public $startStopConfig = true;
    public $username = "vbox";
    public $password = "";
    public $enableAdvancedConfig = false;

    // Automatic config items
    public function __construct()
    {
        $settings = OMVRpc::exec(
            "VirtualBox",
            "getSettings",
            array(),
            array(
                "username" => "admin",
                "role" => OMV_ROLE_ADMINISTRATOR
            ),
            OMV_RPC_MODE_REMOTE
        );

        if (!$settings["enable"]) {
            die("alert('phpVirtualBox disabled by OpenMediaVault configuration.');");
        }

        $this->enableAdvancedConfig = $settings["enable-advanced"];

        $file = new FileInfo("/etc/default/openmediavault-virtualbox");
        $file = $file->openFile();
        $password = $file->fgets();
        $file = null;

        // Trim the password variable since it may contain a newline char
        $this->password = trim($password);
    }
}
