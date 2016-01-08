<?php

/**
 * Copyright (C) 2010-2012 Ian Moore <imooreyahoo@gmail.com>
 * Copyright (C) 2013-2015 OpenMediaVault Plugin Developers.
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
require_once "openmediavault/module.inc";
require_once "openmediavault/notify.inc";
require_once "openmediavault/systemctl.inc";

class OMVModuleVirtualBox extends OMVModuleServiceAbstract implements
    OMVINotifyListener,
    OMVIModuleServiceStatus
{
    private $eventMessagePath = "org.openmediavault.services.virtualbox";
    private $phpvirtualboxMachinesUnit = "phpvirtualbox-machines@vbox";
    private $vboxwebsrvUnit = "virtualbox-web";

    /**
     * Get the service configuration XPath.
     *
     * @return string
     */
    private function getXpath()
    {
        return "/config/services/virtualbox";
    }

    /**
     * Get the module name.
     *
     * @return string
     */
    public function getName()
    {
        return "virtualbox";
    }

    /**
     * Get the module status.
     *
     * @return array
     */
    public function getStatus()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getXpath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        $systemCtl = new OMVSystemCtl($this->vboxwebsrvUnit);

        return [
            "name" => $this->getName(),
            "title" => gettext("VirtualBox Web Service"),
            "enabled" => boolval($object["enable"]),
            "running" => $systemCtl->isActive(),
        ];
    }

    /**
     * Generate the configuration.
     *
     * @return void
     */
    public function applyConfig()
    {
        global $xmlConfig;

        $cmd = "export LANG=C; omv-mkconf virtualbox 2>&1";

        if (0 !== $this->exec($cmd, $output)) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_FAILED,
                $cmd,
                implode(PHP_EOL, $output)
            );
        }
    }

    /**
     * Start the managed service.
     *
     * @return void
     *
     * @throws OMVException
     */
    public function startService()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getXpath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        if (!boolval($object["enable"])) {
            return;
        }

        $systemCtl = new OMVSystemCtl($this->vboxwebsrvUnit);
        $systemCtl->enable(true);
        $systemCtl = new OMVSystemCtl($this->phpvirtualboxMachinesUnit);
        $systemCtl->enable(true);
    }

    /**
     * Stop the managed service.
     *
     * @return void
     */
    public function stopService()
    {
        $systemCtl = new OMVSystemCtl($this->vboxwebsrvUnit);
        $systemCtl->disable(true);
        $systemCtl = new OMVSystemCtl($this->phpvirtualboxMachinesUnit);
        $systemCtl->disable(true);
    }

    /**
     * Bind listeners.
     *
     * @param OMVNotifyDispatcher $dispatcher
     * @return void
     */
    public function bindListeners(OMVNotifyDispatcher $dispatcher)
    {
        $moduleMgr = &OMVModuleMgr::getInstance();

        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            $this->eventMessagePath,
            [$this, "setDirty"]
        );

        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            $this->eventMessagePath,
            [$moduleMgr->getModule("php5fpm"), "setDirty"]
        );

        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            $this->eventMessagePath,
            [$moduleMgr->getModule("webserver"), "setDirty"]
        );
    }
}

// Register the module.
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleVirtualBox());
