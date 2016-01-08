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
require_once "openmediavault/notify.inc";
require_once "openmediavault/rpcservice.inc";

require_once "/usr/share/phpvirtualbox/endpoints/lib/config.php";
require_once "/usr/share/phpvirtualbox/endpoints/lib/utils.php";
require_once "/usr/share/phpvirtualbox/endpoints/lib/vboxconnector.php";

class OMVRpcServiceVirtualBox extends OMVRpcServiceAbstract
{
    private $eventMessagePath = "org.openmediavault.services.virtualbox";

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
     * Get the name of the RPC service.
     *
     * @return string
     */
    public function getName()
    {
        return "VirtualBox";
    }

    /**
     * Initialize the RPC service.
     *
     * @return void
     */
    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getMachines");
        $this->registerMethod("getMachine");
        $this->registerMethod("setMachine");
        $this->registerMethod("setMachineState");
    }

    /**
     * Get the settings of the service.
     *
     * @param array $params The method paramaters.
     * @param array $context The context of the caller.
     * @return array
     *
     * @throws OMVException
     */
    public function getSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR,
        ]);

        // Get the configuration object.
        $object = $xmlConfig->get($this->getXpath());
        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        // Modify the result data.
        $object["enable"] = boolval($object["enable"]);
        $object["enable_advanced"] = boolval($object["enable_advanced"]);

        return $object;
    }

    /**
     * Set the settings of the service.
     *
     * @param array $params The method paramaters.
     * @param array $context The context of the caller.
     * @return array
     *
     * @throws OMVException
     */
    public function setSettings($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR,
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "enable": { "type": "boolean" },
                    "machines.sharedfolderref": { '.$GLOBALS["OMV_JSONSCHEMA_UUID"].' },
                    "enable_advanced": { "type": "boolean" }
                }
            }'
        );

        // Prepare the configuration data.
        $object = [
            "enable" => boolval($params["enable"]),
            "machines.sharedfolderref" => $params["machines.sharedfolderref"],
            "enable_advanced" => boolval($params["enable_advanced"]),
        ];

        // Set the configuration object.
        if (false === $xmlConfig->replace($this->getXpath(), $object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXpath()
            );
        }

        // Notify about the configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->eventMessagePath,
            $object
        );

        return $object;
    }

    /**
     * Get a list of available machines.
     *
     * @param array $params The method paramaters.
     * @param array $context The context of the caller.
     * @return array
     *
     * @throws OMVException
     */
    public function getMachines($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR,
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "start": { "type": "integer" },
                    "limit": { '.$GLOBALS["OMV_JSONSCHEMA_COUNTFIELD"].' },
                    "sortfield": { '.$GLOBALS["OMV_JSONSCHEMA_SORTFIELD"].' },
                    "sortdir": { '.$GLOBALS["OMV_JSONSCHEMA_SORTDIR"].' }
                }
            }'
        );

        $settings = $this->getSettings([], $context);
        $objects = [];

        // Only connect to VirtualBox and get the machines if the plugin
        // is enabled since there's no vboxwebsrv instance running when the
        // plugin is disabled.
        //
        // FIXME: We should check if vboxwebsrv is running.
        if ($settings["enable"]) {
            $vb = new vboxconnector();
            $vb->skipSessionCheck = true;
            $vb->connect();

            foreach ($vb->vbox->machines as $machine) {
                $objects[] = [
                    "uuid" => $machine->id,
                    "name" => $machine->name,
                    "state" => $machine->state->__toString(),
                    "startupMode" => $machine->getExtraData("pvbx/startupMode"),
                    "OSTypeId" => $machine->getOSTypeId(),
                    "sessionState" => $machine->sessionState->__toString(),
                ];
            }

            unset($vb);
        }

        return $this->applyFilter(
            $objects,
            $params["start"],
            $params["limit"],
            $params["sortfield"],
            $params["sortdir"]
        );
    }

    /**
     * Get a machine by its ID.
     *
     * @param array $params The method paramaters.
     * @param array $context The context of the caller.
     * @return array
     *
     * @throws OMVException
     */
    public function getMachine($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR,
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "uuid": { '.$GLOBALS["OMV_JSONSCHEMA_UUID_NONE"].' }
                }
            }'
        );

        $settings = $this->getSettings([], $context);
        $object = [];

        // Only connect to VirtualBox if the plugin is enabled since vboxwebsrv
        // is not running when the plugin is disabled.
        if ($settings["enable"]) {
            $vb = new vboxconnector();
            $vb->skipSessionCheck = true;
            $vb->connect();

            $machine = $vb->vbox->findMachine($params["uuid"]);
            $object = [
                "uuid" => $machine->id,
                "name" => $machine->name,
                "state" => $machine->state->__toString(),
                "startupMode" => $machine->getExtraData("pvbx/startupMode"),
                "OSTypeId" => $machine->getOSTypeId(),
                "sessionState" => $machine->sessionState->__toString(),
            ];

            unset($vb);
        }

        return $object;
    }

    /**
     * Set a machine.
     *
     * @param array $params The method paramaters.
     * @param array $context The context of the caller.
     * @return void
     *
     * @throws OMVException
     */
    public function setMachine($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR,
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "uuid": {'.$GLOBALS["OMV_JSONSCHEMA_UUID_NONE"].'},
                    "name": { "type": "string", "optional": true },
                    "startupMode": { "type": "string", "enum": ["auto", "manual"] }
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

    /**
     * Set the machine state of a machine.
     *
     * @param array $params The method paramaters.
     * @param array $context The context of the caller.
     * @return mixed
     *
     * @throws OMVException
     */
    public function setMachineState($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR,
        ]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "uuid": { '.$GLOBALS["OMV_JSONSCHEMA_UUID_NONE"].' },
                    "state": { "type": "string" }
                }
            }'
        );

        $vb = new vboxconnector();
        $vb->skipSessionCheck = true;

        $args = [
            "vm" => $params["uuid"],
            "state" => $params["state"],
        ];

        $response = [];
        $vb->machineSetState($args, [&$response]);
        unset($vb);

        return $response["data"];
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceVirtualBox());
