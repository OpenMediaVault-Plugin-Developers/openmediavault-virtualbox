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

class phpVBoxConfig
{

    /* STATIC CONFIG ITEMS */
    public $location = 'http://127.0.0.1:18083/';
    public $language = 'en';
    public $startStopConfig = true;
    public $username = '';
    public $password = '';
    public $enableAdvancedConfig = false;

    /* AUTOMATIC CONFIG ITEMS */
    public function __construct()
    {
        $out = array();
        exec('/bin/sh -c \'. /etc/default/openmediavault; . /usr/share/openmediavault/scripts/helper-functions; echo $(omv_config_get "//services/virtualbox/enable"); OMV_VBOX_USER=${OMV_VBOX_USER:-"vbox"}; echo ${OMV_VBOX_USER}; cat /etc/default/openmediavault-virtualbox; echo $(omv_config_get "//services/virtualbox/enable-advanced")\'', $out);
        if($out[0] != "1") die("alert('phpVirtualBox disabled by OpenMediaVault configuration.');");
        $this->username = $out[1];
        $this->password = $out[2];
        $this->enableAdvancedConfig = ($out[3] == "1");
    }

}
