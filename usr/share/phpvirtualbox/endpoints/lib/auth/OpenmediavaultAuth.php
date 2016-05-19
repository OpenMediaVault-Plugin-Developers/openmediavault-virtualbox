<?php

/**
 * Copyright (C) 2016 OpenMediaVault Plugin Developers.
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

require_once '/usr/share/php/openmediavault/autoloader.inc';
require_once '/usr/share/php/openmediavault/globals.inc';

use OMV\Rpc\Rpc;

class PhpvbAuthOpenmediavaultAuth implements phpvbAuth
{
    public $capabilities = [
        'canChangePassword' => false,
        'canLogout' => true,
        'canModifyUsers' => false,
    ];

    public function login($username, $password)
    {
        $omvRpcContext = [
            'username' => 'admin',
            'role' => OMV_ROLE_ADMINISTRATOR,
        ];

        try {
            $result = Rpc::call('UserMgmt', 'authUser', [
                'username' => $username,
                'password' => $password,
            ], $omvRpcContext, OMV_RPC_MODE_REMOTE);

            // Return early.
            if (!$result['authenticated']) {
                return;
            }

            $user = Rpc::call('UserMgmt', 'getUser', [
                'name' => $username,
            ], $omvRpcContext, OMV_RPC_MODE_REMOTE);

            // Only allow admin or users in the vboxusers group.
            if ($username === 'admin' || in_array('vboxusers', $user['groups'])) {
                $_SESSION['admin'] = true;
                $_SESSION['valid'] = true;
                $_SESSION['user'] = $username;
            }
        } catch (Exception $e) {
            // Do nothing.
        }
    }

    public function changePassword($old, $new)
    {
        //
    }

    public function heartbeat($vbox)
    {
        //
    }

    public function logout(&$response)
    {
        $_SESSION = [];
        session_destroy();

        $response['data']['result'] = 1;
    }

    public function listUsers()
    {
        //
    }

    public function updateUser($vboxRequest, $skipExistCheck)
    {
        //
    }

    public function deleteUser($user)
    {
        //
    }
}
