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

namespace OMV\Engined\Rpc;

use OMV\Config\Database;
use OMV\Engine\Notify\Dispatcher;
use OMV\Rpc\ServiceAbstract;
use vboxconnector as VirtualboxConnector;

class VirtualBox extends ServiceAbstract
{
    /** @var Database */
    private $database;

    /** @var string */
    private $dataModelPath = 'conf.service.virtualbox';

    /** @var Dispatcher */
    private $dispatcher;

    /** @var string */
    private $eventMessagePath = 'org.openmediavault.conf.service.virtualbox';

    public function __construct(Database $database = null, Dispatcher $dispatcher = null)
    {
        $this->database = $database ?: Database::getInstance();
        $this->dispatcher = $dispatcher ?: Dispatcher::getInstance();
    }

    /**
     * Get the name of the RPC service.
     *
     * @return string
     */
    public function getName()
    {
        return 'VirtualBox';
    }

    /**
     * Initialize the RPC service.
     *
     * @return void
     */
    public function initialize()
    {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
        $this->registerMethod('getMachines');
        $this->registerMethod('getMachine');
        $this->registerMethod('setMachine');
        $this->registerMethod('setMachineState');
    }

    /**
     * Get the settings of the service.
     *
     * @param array $params  The method paramaters.
     * @param array $context The context of the caller.
     *
     * @return array
     */
    public function getSettings($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $object = $this->database->get($this->dataModelPath);

        return $object->getAssoc();
    }

    /**
     * Set the settings of the service.
     *
     * @param array $params  The method paramaters.
     * @param array $context The context of the caller.
     *
     * @return array
     */
    public function setSettings($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.virtualbox.setsettings');

        $object = $this->database->get($this->dataModelPath);
        $object->setAssoc($params);

        $this->database->set($object);
        $this->dispatcher->notify(OMV_NOTIFY_MODIFY, $this->eventMessagePath, $object);

        return $object->getAssoc();
    }

    /**
     * Get a list of available machines.
     *
     * @param array $params  The method paramaters.
     * @param array $context The context of the caller.
     *
     * @return array
     */
    public function getMachines($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.common.getlist');

        $objects = [];
        $settings = $this->database->get($this->dataModelPath);

        // Only connect to VirtualBox and get the machines if the plugin
        // is enabled since there's no vboxwebsrv instance running when the
        // plugin is disabled.
        //
        // FIXME: We should check if vboxwebsrv is running.
        if ($settings->get('enable')) {
            $connector = new VirtualboxConnector();
            $connector->skipSessionCheck = true;
            $connector->connect();

            foreach ($connector->vbox->machines as $machine) {
                $objects[] = [
                    'uuid' => $machine->id,
                    'name' => $machine->name,
                    'state' => $machine->state->__toString(),
                    'startupMode' => $machine->getExtraData('pvbx/startupMode'),
                    'OSTypeId' => $machine->getOSTypeId(),
                    'sessionState' => $machine->sessionState->__toString(),
                ];
            }

            unset($connector);
        }

        return $this->applyFilter(
            $objects,
            $params['start'],
            $params['limit'],
            $params['sortfield'],
            $params['sortdir']
        );
    }

    /**
     * Get a machine by its ID.
     *
     * @param array $params  The method paramaters.
     * @param array $context The context of the caller.
     *
     * @return array
     */
    public function getMachine($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.common.objectuuid');

        $settings = $this->database->get($this->dataModelPath);

        // Only connect to VirtualBox and get the machines if the plugin
        // is enabled since there's no vboxwebsrv instance running when the
        // plugin is disabled.
        //
        // FIXME: We should check if vboxwebsrv is running.
        if ($settings->get('enable')) {
            $connector = new VirtualboxConnector();
            $connector->skipSessionCheck = true;
            $connector->connect();

            $machine = $connector->vbox->findMachine($params['uuid']);

            $object = [
                'uuid' => $machine->id,
                'name' => $machine->name,
                'state' => $machine->state->__toString(),
                'startupMode' => $machine->getExtraData('pvbx/startupMode'),
                'OSTypeId' => $machine->getOSTypeId(),
                'sessionState' => $machine->sessionState->__toString(),
            ];

            unset($connector);

            return $object;
        }
    }

    /**
     * Set a machine.
     *
     * @param array $params  The method paramaters.
     * @param array $context The context of the caller.
     *
     * @return void
     */
    public function setMachine($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.virtualbox.setmachine');

        $connector = new VirtualboxConnector();
        $connector->skipSessionCheck = true;
        $connector->connect();

        $machine = $connector->vbox->findMachine($params['uuid']);
        $isMachineRunning = ($machine->state->__toString() == 'Running');

        $connector->session = $connector->websessionManager->getSessionObject($connector->vbox->handle);
        $machine->lockMachine($connector->session->handle, ($isMachineRunning ? 'Shared' : 'Write'));

        $sessionMachine = $connector->session->machine;

        if (!$isMachineRunning && !empty($params['name']) && $sessionMachine->name !== $params['name']) {
            $sessionMachine->name = $params['name'];
        }

        $sessionMachine->setExtraData('pvbx/startupMode', $params['startupMode']);

        $connector->session->machine->saveSettings();
        $connector->session->unlockMachine();
        $connector->session->releaseRemote();

        unset($connector->session);
        unset($connector);
    }

    /**
     * Set the machine state of a machine.
     *
     * @param array $params  The method paramaters.
     * @param array $context The context of the caller.
     *
     * @return mixed
     */
    public function setMachineState($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.virtualbox.setmachinestate');

        $connector = new VirtualboxConnector();
        $connector->skipSessionCheck = true;

        $response = [];

        $connector->machineSetState(
            ['vm' => $params['uuid'], 'state' => $params['state']],
            [&$response]
        );

        unset($connector);

        return $response['data'];
    }
}
