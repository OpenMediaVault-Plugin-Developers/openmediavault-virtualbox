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
        this.on("load", function () {
            var checked = this.findField("enable").checked;
            var showTab = this.findField("show_tab").checked;
            var parent = this.up("tabpanel");

            if (!parent) {
                return;
            }

            var gridPanel = parent.down("grid");
            var phpVirtualBoxPanel = parent.down("panel[title=" + _("phpVirtualBox") + "]");

            if (gridPanel) {
                if (checked) {
                    gridPanel.enable();
                } else {
                    gridPanel.disable();
                }
            }

            if (phpVirtualBoxPanel) {
                if (checked) {
                    phpVirtualBoxPanel.enable();
                } else {
                    phpVirtualBoxPanel.disable();
                }

                if (showTab) {
                    phpVirtualBoxPanel.tab.show();
                } else {
                    phpVirtualBoxPanel.tab.hide();
                }
            }
        }, this);

        me.callParent(arguments);
    },

    getFormItems : function() {
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
                xtype      : "sharedfoldercombo",
                name       : "machines.sharedfolderref",
                fieldLabel : _("Data directory"),
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("The location where MySQL stores its data.")
                }]
            },{
                xtype   : "button",
                name    : "fixmodule",
                text    : _("Fix module for backports kernels"),
                scope   : this,
                handler : Ext.Function.bind(this.onFixModuleButton, this)
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
                border : false,
                html   : "<p>" + _("Make sure to change the password in phpVirtualBox! The default login credentials are 'admin' for both the username and password.") + "</p>"
            },{
                xtype      : "checkbox",
                name       : "enable_advanced",
                fieldLabel : _("Advanced configuration"),
                boxLabel   : _("Show advanced configuration options in phpVirtualBox web interface."),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "show_tab",
                fieldLabel : _("Show Tab"),
                boxLabel   : _("Show tab containing phpVirtualBox frame."),
                checked    : false
            }]
        }];
    },

    onFixModuleButton : function() {
        Ext.create("OMV.window.Execute", {
            title          : _("Recompile vboxdrv module for 3.2 kernel ..."),
            rpcService     : "VirtualBox",
            rpcMethod      : "fixModule",
            hideStopButton : true,
            listeners      : {
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
