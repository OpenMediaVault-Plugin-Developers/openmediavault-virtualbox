/**
 * Copyright (C) 2010-2012 Ian Moore <imooreyahoo@gmail.com>
 * Copyright (C) 2013-2017 OpenMediaVault Plugin Developers
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
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/form/Panel.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")

Ext.define('OMV.module.admin.service.virtualbox.Settings', {
    extend: 'OMV.workspace.form.Panel',
    uses: [
        'OMV.data.Model',
        'OMV.data.Store'
    ],

    rpcService: 'VirtualBox',
    rpcGetMethod: 'getSettings',
    rpcSetMethod: 'setSettings',

    getButtonItems: function() {
        var items = this.callParent(arguments);

        items.push({
            id: this.getId() + '-phpvirtualbox',
            xtype: 'button',
            text: _('phpVirtualBox'),
            icon: 'images/virtualbox.png',
            iconCls: Ext.baseCSSPrefix + 'btn-icon-16x16',
            scope: this,
            handler: function() {
                window.open('/virtualbox/', '_blank');
            }
        });

        return items;
    },

    getFormItems: function() {
        return [{
            xtype: 'fieldset',
            title: 'General settings',
            defaults: {
                labelSeparator: ''
            },
            items: [{
                xtype: 'checkbox',
                name: 'enable',
                fieldLabel: _('Enable'),
                checked: false
            }, {
                xtype: 'sharedfoldercombo',
                name: 'sharedfolderref',
                fieldLabel: _('VM directory'),
                plugins: [{
                    ptype: 'fieldinfo',
                    text: _('The location where VirtualBox stores its virtual machines.')
                }]
            }]
        }, {
            xtype: 'fieldset',
            title: 'phpVirtualBox',
            defaults: {
                labelSeparator: ''
            },
            items: [{
                xtype: 'displayfield',
                fieldLabel: 'Information',
                value: _('To log in to phpVirtualBox use the credentials of the admin user or a user in the group vboxusers.')
            }, {
                xtype: 'checkbox',
                name: 'enable_advanced',
                fieldLabel: _('Advanced configuration'),
                boxLabel: _('Show advanced configuration options in phpVirtualBox web interface.'),
                checked: false
            }]
        }];
    },
});


OMV.WorkspaceManager.registerPanel({
    id: 'settings',
    path: '/service/virtualbox',
    text: _('Settings'),
    position: 10,
    className: 'OMV.module.admin.service.virtualbox.Settings'
});
