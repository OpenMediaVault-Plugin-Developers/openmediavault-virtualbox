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


require_once "/usr/share/phpvirtualbox/lib/config.php";
require_once "/usr/share/phpvirtualbox/lib/utils.php";
require_once "/usr/share/phpvirtualbox/lib/vboxconnector.php";
require_once "openmediavault/error.inc";
require_once "openmediavault/notify.inc";
require_once "openmediavault/object.inc";
require_once "openmediavault/rpc.inc";
require_once "openmediavault/util.inc";

class OMVRpcServiceVirtualBox extends OMVRpcServiceAbstract
{
    private function getXpath()
    {
        return "//services/virtualbox";
    }

    public function getName()
    {
        return "VirtualBox";
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getMachines");
        $this->registerMethod("getMachine");
        $this->registerMethod("setMachine");
        $this->registerMethod("setMachineState");
        $this->registerMethod("fixModule");
    }

    /**
     * Get main settings
     */
    public function getSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Get configuration object
        $object = $xmlConfig->get($this->getXpath());
        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        // Modify result data
        $object["enable"]          = boolval($object["enable"]);
        $object["enable_advanced"] = boolval($object["enable_advanced"]);
        $object["show_tab"]        = boolval($object["show_tab"]);

        return $object;
    }

    /**
     * Set main settings
     */
    public function setSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "enable"          : { "type" : "boolean" },
                    "enable_advanced" : { "type" : "boolean" },
                    "vm_folder"       : { "type" : "string", "optional" : true },
                    "mntentref"       : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . ' },
                    "show_tab"        : { "type" : "boolean" }
                }
            }'
        );

        $xpath = sprintf("//system/fstab/mntent[uuid='%s']", $params["mntentref"]);
        $deviceDir = $xmlConfig->get($xpath);
        $deviceDir = $deviceDir["dir"];

        $vmFolder = "{$deviceDir}/virtualbox-machines";
        $params["vm_folder"] = $vmFolder;

        $oldConfig = $xmlConfig->get($this->getXpath());

        if ($oldConfig["vm_folder"] != $vmFolder) {
            $output = array();
            $cmd = "sudo /bin/sh -c '. /etc/default/openmediavault; OMV_VBOX_USER=\${OMV_VBOX_USER:-\"vbox\"}; echo \${OMV_VBOX_USER}'";
            $this->exec($cmd, $output);
            $vboxUser = $output[0];
            $cmd = "sudo /bin/sh -c '[ -d $vmFolder ] || /bin/mkdir -p $vmFolder; chown $vboxUser $vmFolder; chmod u+rwx $vmFolder'";
            $result = $this->exec($cmd, $output);

            if ($result !== 0) {
                throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
            }
        }

        // Prepare configuration data
        $object = array(
            "enable"          => array_boolval($params, "enable"),
            "enable_advanced" => array_boolval($params, "enable_advanced"),
            "vm_folder"       => $params["vm_folder"],
            "mntentref"       => $params["mntentref"],
            "show_tab"        => array_boolval($params, "show_tab")
        );

        // Set configuration object
        if (false === $xmlConfig->replace($this->getXpath(), $object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY, "org.openmediavault.services.virtualbox", $object);

        return $object;
    }

    public function getMachines($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "start"     : { "type" : "integer" },
                    "limit"     : { ' . $GLOBALS["OMV_JSONSCHEMA_COUNTFIELD"] . ' },
                    "sortfield" : { ' . $GLOBALS["OMV_JSONSCHEMA_SORTFIELD"] . ' },
                    "sortdir"   : { ' . $GLOBALS["OMV_JSONSCHEMA_SORTDIR"] . ' }
                }
            }'
        );

        $settings = $this->getSettings(array(), $context);
        $objects = array();

        // Only connect to VirtualBox and get the machines if the plugin is
        // enabled. Otherwise it seems to consume a lot of resources doing
        // something that fails.
        //
        // We should check the vboxweb-service status but it returns a 0
        // even when the plugin is not running.
        if ($settings["enable"]) {
            $vb = new vboxconnector();
            $vb->skipSessionCheck = true;
            $vb->connect();

            foreach ($vb->vbox->machines as $machine) {
                $objects[] = array(
                    "uuid"         => $machine->id,
                    "name"         => $machine->name,
                    "state"        => $machine->state->__toString(),
                    "startupMode"  => $machine->getExtraData("pvbx/startupMode"),
                    "OSTypeId"     => $machine->getOSTypeId(),
                    "sessionState" => $machine->sessionState->__toString()
                );
            }

            unset($vb);
        }

        return $this->applyFilter(
            $objects,
            $params["start"],
            $params["count"],
            $params["sortfield"],
            $params["sortdir"]
        );
    }

    public function getMachine($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid" : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . ' }
                }
            }'
        );

        $settings = $this->getSettings(array(), $context);
        $object = array();

        // Only connect to VirtualBox if the plugin is enabled since an
        // unsuccessful request will for some reason utilize a lot of
        // resources.
        if ($settings["enable"]) {
            $vb = new vboxconnector();
            $vb->skipSessionCheck = true;
            $vb->connect();

            $machine = $vb->vbox->findMachine($params["uuid"]);
            $object = array(
                "uuid"         => $machine->id,
                "name"         => $machine->name,
                "state"        => $machine->state->__toString(),
                "startupMode"  => $machine->getExtraData("pvbx/startupMode"),
                "OSTypeId"     => $machine->getOSTypeId(),
                "sessionState" => $machine->sessionState->__toString()
            );

            unset($vb);
        }

        return $object;
    }

    public function setMachine($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid"        : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . ' },
                    "name"        : { "type" : "string", "optional" : true },
                    "startupMode" : { "type" : "string", "enum" : [ "auto", "manual" ] }
                }
            }'
        );

        $vb = new vboxconnector();
        $vb->skipSessionCheck = true;
        $vb->connect();

        $machine = $vb->vbox->findMachine($params["uuid"]);
        $vmRunning = ($machine->state->__toString() == "Running");

        $vb->session = $vb->websessionManager->getSessionObject($vb->vbox->handle);
        $machine->lockMachine($vb->session->handle, ($vmRunning ? "Shared" : "Write"));

        $m = &$vb->session->machine;

        if (!$vmRunning &&
            isset($params["name"]) &&
            !empty($params["name"]) &&
            $m->name != $params["name"]
        ) {
            $m->name = $params["name"];
        }

        $m->setExtraData("pvbx/startupMode", $params["startupMode"]);

        $vb->session->machine->saveSettings();
        $vb->session->unlockMachine();
        $vb->session->releaseRemote();
        unset($vb->session);
        unset($vb);
    }

    public function setMachineState($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid"  : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID_NONE"] . ' },
                    "state" : { "type" : "string" }
                }
            }'
        );

        $vb = new vboxconnector();
        $vb->skipSessionCheck = true;

        $args = array(
            "vm"    => $params["uuid"],
            "state" => $params["state"]
        );

        $response = array();
        $vb->machineSetState($args, array(&$response));
        unset($vb);

        return $response["data"];
    }

    public function fixModule($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if ($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            $cmd = "/usr/local/bin/fixmodule";
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
        } catch (Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceVirtualBox());
