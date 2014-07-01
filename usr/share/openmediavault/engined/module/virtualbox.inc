<?php

/**
 * Copyright (C) 2010-2012 Ian Moore <imooreyahoo@gmail.com>
 * Copyright (C) 2013-2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/initscript.inc";
require_once "openmediavault/module.inc";
require_once "openmediavault/util.inc";

class OMVModuleVirtualBox extends OMVModuleServiceAbstract implements
    OMVINotifyListener,
    OMVIModuleServiceStatus
{
    private function getEventMessagePath()
    {
        return "org.openmediavault.services.virtualbox";
    }

    private function getXPath()
    {
        return "//services/virtualbox";
    }

    /**
     * Get the module name.
     */
    public function getName()
    {
        return "virtualbox";
    }

    /**
     * Get the module status
     */
    public function getStatus()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getXPath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        $cmd = "sudo /etc/init.d/vboxweb-service status";
        $this->exec($cmd, $output);

        return array(
            "name"    => $this->getName(),
            "title"   => gettext("VirtualBox WebService"),
            "enabled" => boolval($object["enable"]),
            "running" => (strpos($output[0], "...running") > 0)
        );
    }

    /**
     * Create and update the configuration
     */
    public function applyConfig()
    {
        global $xmlConfig;

        $cmd = "sudo omv-mkconf virtualbox 2>&1";

        if (0 !== $this->exec($cmd, $output)) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_FAILED,
                $cmd,
                implode("\n", $output)
            );
        }
    }

    /**
     * Start the managed service
     */
    public function startService()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getXPath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        if (boolval($object["enable"]) === true) {
            $initd = new OMVSysVInitScript("vboxweb-service", true);
            $initd->exec();
            $initd = new OMVSysVInitScript("openmediavault-virtualbox", true);
            $initd->exec();
        }
    }

    /**
     * Stop the managed service
     */
    public function stopService()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getXPath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        if (boolval($object["enable"]) === false) {
            $initd = new OMVSysVInitScript("openmediavault-virtualbox", false);
            $initd->exec();
            $initd = new OMVSysVInitScript("vboxweb-service", false);
            $initd->exec();
        }

        $cmd = "/bin/rm -f /etc/default/virtualbox";

        if (0 !== $this->exec($cmd, $output)) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_FAILED,
                $cmd,
                implode("\n", $output)
            );
        }
    }

    /**
     * Bind listeners.
     */
    public function bindListeners(OMVNotifyDispatcher $dispatcher)
    {
        $moduleMgr = &OMVModuleMgr::getInstance();

        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            array($this, "setDirty")
        );

        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            array($moduleMgr->getModule("webserver"), "setDirty")
        );
    }
}

// Register module
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleVirtualBox());
