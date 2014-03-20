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
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/form/Panel.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")

Ext.define("OMV.module.admin.service.virtualbox.Settings", {
    extend : "OMV.workspace.form.Panel",
    uses   : [
        "OMV.data.Model",
        "OMV.data.Store"
    ],

    rpcService   : "VirtualBox",
    rpcGetMethod : "getSettings",
    rpcSetMethod : "setSettings",

    initComponent : function() {
        var me = this;

        me.on("load", function () {
            var checked = me.findField("enable").checked;
            var showtab = me.findField("showtab").checked;
            var parent = me.up("tabpanel");

            if (!parent)
                return;

            var gridPanel = parent.down("grid");
            var phpVirtualBoxPanel = parent.down("panel[title=" + _("phpVirtualBox") + "]");

            if (gridPanel)
                checked ? gridPanel.enable() : gridPanel.disable();
            if (phpVirtualBoxPanel) {
                checked ? phpVirtualBoxPanel.enable() : phpVirtualBoxPanel.disable();
                showtab ? phpVirtualBoxPanel.tab.show() : phpVirtualBoxPanel.tab.hide();
            }
        });

        me.callParent(arguments);
    },

    getFormItems : function() {
        var me = this;
        return [{
            xtype    : "fieldset",
            title    : "General settings",
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "checkbox",
                name       : "enable",
                fieldLabel : _("Enable"),
                checked    : false
            },{
                xtype         : "combo",
                name          : "mntentref",
                fieldLabel    : _("Virtual Machine Volume"),
                emptyText     : _("Select a volume ..."),
                allowBlank    : false,
                allowNone     : false,
                editable      : false,
                triggerAction : "all",
                displayField  : "description",
                valueField    : "uuid",
                store         : Ext.create("OMV.data.Store", {
                    autoLoad : true,
                    model    : OMV.data.Model.createImplicit({
                        idProperty : "uuid",
                        fields     : [
                            { name : "uuid", type : "string" },
                            { name : "devicefile", type : "string" },
                            { name : "description", type : "string" }
                        ]
                    }),
                    proxy : {
                        type : "rpc",
                        rpcData : {
                            service : "ShareMgmt",
                            method  : "getCandidates"
                        },
                        appendSortParams : false
                    },
                    sorters : [{
                        direction : "ASC",
                        property  : "devicefile"
                    }]
                })
            },{
                xtype      : "textfield",
                name       : "vm-folder",
                fieldLabel : _("Virtual Machine Folder"),
                allowNone  : true,
                readOnly   : true
            },{
                border : false,
                html   : "<br />"
            },{
                xtype   : "button",
                name    : "fixmodule",
                text    : _("Fix module for backports kernels"),
                scope   : this,
                handler : Ext.Function.bind(me.onFixModuleButton, me, [ me ])
            },{
                border : false,
                html   : "<ul><li>" + _("This will recompile the vboxdrv for the backports kernel.") + "</li></ul>"
            }]
        },{
            xtype    : "fieldset",
            title    : "phpVirtualBox",
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "checkbox",
                name       : "enable-advanced",
                fieldLabel : _("Advanced configuration"),
                boxLabel: _("Show advanced configuration options in phpVirtualBox web interface."),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "showtab",
                fieldLabel : _("Show Tab"),
                boxLabel: _("Show tab containing phpVirtualBox frame."),
                checked    : false
            }]
        },{
            xtype  : "fieldset",
            layout : "fit",
            title  : _("Note"),
            items  : [{
                border : false,
                html   : _("Make sure to change the password in phpVirtualBox! The default login credentials are 'admin' for both the username and password.") + "<br /><br />"
            }]
        }];
    },

    onFixModuleButton : function() {
        var me = this;
        Ext.create("OMV.window.Execute", {
            title          : _("Recompile vboxdrv module for 3.2 kernel ..."),
            rpcService     : "VirtualBox",
            rpcMethod      : "fixModule",
            hideStopButton : true,
            listeners      : {
                scope     : me,
                exception : function(wnd, error) {
                    OMV.MessageBox.error(null, error);
                }
            }
        }).show();
    }
});


OMV.WorkspaceManager.registerPanel({
    id        : "settings",
    path      : "/service/virtualbox",
    text      : _("Settings"),
    position  : 10,
    className : "OMV.module.admin.service.virtualbox.Settings"
});

