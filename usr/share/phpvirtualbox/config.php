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

use Exception;
use SplFileInfo as FileInfo;

class phpVBoxConfig
{
    // Static config items
    public $location = "http://127.0.0.1:18083/";
    public $language = "en";
    public $startStopConfig = true;
    public $username = "";
    public $password = "";
    public $enableAdvancedConfig = false;

    // Automatic config items
    public function __construct()
    {
        $settings = $this->getSettings();

        if (!$settings->enable) {
            die("alert('phpVirtualBox disabled by OpenMediaVault configuration.');");
        }

        $this->enableAdvancedConfig = $settings->enable_advanced;
        $this->username = $this->getVboxUserUsername();
        $this->password = $this->getVboxUserPassword();
    }

    private function getSettings()
    {
        $data = shell_exec("/usr/sbin/omv-rpc VirtualBox getSettings");

        if (!$data) {
            throw new Exception("Failed to get OMV VirtualBox settings.");
        }

        return json_decode($data);
    }

    private function getVboxUserUsername()
    {
        return "vbox";
    }

    private function getVboxUserPassword()
    {
        $file = new FileInfo("/etc/default/openmediavault-virtualbox");

        if (!$file->isReadable()) {
            throw new Exception("Can't read /etc/defaults/openmediavault-virtualbox");
        }

        $file = $file->openFile();
        $password = $file->fgets();

        $file = null;

        // Trim the password variable since it may contain a newline char
        return trim($password);
    }
}
