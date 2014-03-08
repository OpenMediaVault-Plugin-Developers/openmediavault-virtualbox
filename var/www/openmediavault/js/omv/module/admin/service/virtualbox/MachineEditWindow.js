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
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")

Ext.define("OMV.module.admin.service.virtualbox.MachineEditWindow", {
    extend : "OMV.workspace.window.Form",
    uses   : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    rpcService   : "VirtualBox",
    rpcGetMethod : "getMachine",
    rpcSetMethod : "setMachine",

    hideResetButton : true,
    title           : _("Edit Virtual Machine"),

    plugins: [{
        ptype: "configobject"
    }],

    getFormItems : function() {
        var me = this;

        return [{
            xtype      : "textfield",
            name       : "name",
            fieldLabel : _("Name"),
            allowBlank : false
        },{
            xtype      : "combo",
            name       : "startupMode",
            fieldLabel : _("Startup Mode"),
            queryMode  : "local",
            store      : Ext.create("Ext.data.SimpleStore", {
                fields : [
                    "value",
                    "text"
                ],
                data : [
                    [ "manual", _("Manual") ],
                    [ "auto", _("Automatic") ]
                ]
            }),
            displayField  : "text",
            valueField    : "value",
            allowBlank    : false,
            editable      : false,
            triggerAction : "all",
            value         : Ext.isEmpty(me.startupMode) ? "manual" : me.startupMode
        }];
    },

    initComponent : function() {
        var me = this;

        me.callParent(arguments);

        me.on("load", function() {
            if (me.sessionState != "Unlocked")
                me.findField("name").setReadOnly(true);
        }, me);
    }

});
