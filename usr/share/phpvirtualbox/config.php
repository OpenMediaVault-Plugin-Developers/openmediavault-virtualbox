<?php
/**
 * Copyright (C) 2010-2012 Ian Moore <imooreyahoo@gmail.com>
 * Copyright (C) 2013-2015 OpenMediaVault Plugin Developers
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

class phpVBoxConfig
{
    /**
     * The SOAP url used to connect to vboxwebsrv.
     *
     * @var string
     */
    public $location = "http://127.0.0.1:18083/";

    /**
     * The username for the system user that runs VirtualBox.
     *
     * @var string
     */
    public $username = "";

    /**
     * The password for the system user that runs VirtualBox.
     *
     * @var string
     */
    public $password = "";

    /**
     * The authentication library to use.
     *
     * @var string
     */
    public $authLib = "OpenmediavaultAuth";

    /**
     * Whether to prompt about deleting hard disk files when removing files from
     * the Virtual Media Manager. If not set or false files will always be kept.
     *
     * @var bool
     */
    public $deleteOnRemove = true;

    /**
     * Whether to enable advanced configuration items in phpVirtualBox.
     *
     * @var bool
     */
    public $enableAdvancedConfig = false;

    /**
     * The default language.
     *
     * @var string
     */
    public $language = "en";

    /**
     * Enable option to autmatically start and stop VMs when booting and
     * shutting down.
     *
     * @var bool
     */
    public $startStopConfig = true;

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

    /**
     * Returns the plugin settings from OpenMediaVault.
     *
     * @return object
     */
    private function getSettings()
    {
        // Normally we should use OMVRpc::exec for this but since pulling
        // it in caused issues in OMV we just call the RPC from the shell.
        $data = shell_exec("/usr/sbin/omv-rpc VirtualBox getSettings");

        if (!$data) {
            throw new Exception("Failed to get OMV VirtualBox settings.");
        }

        return json_decode($data);
    }

    /**
     * Returns the username for the user which VirtualBox is running with.
     *
     * @return string
     */
    private function getVboxUserUsername()
    {
        return "vbox";
    }

    /**
     * Returns the password for the user which VirtualBox is running with.
     *
     * @return string
     */
    private function getVboxUserPassword()
    {
        $file = new SplFileInfo("/etc/default/openmediavault-virtualbox");

        if (!$file->isReadable()) {
            throw new Exception("Can't read /etc/defaults/openmediavault-virtualbox");
        }

        $file = $file->openFile();
        $password = $file->fgets();

        $file = null;

        // Trim the password variable since it may contain a newline character.
        return trim($password);
    }
}
