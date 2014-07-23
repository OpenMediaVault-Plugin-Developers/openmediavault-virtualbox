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
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/module/admin/service/virtualbox/MachineEditWindow.js")

Ext.define("OMV.module.admin.service.virtualbox.Machines", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.module.admin.service.virtualbox.MachineEditWindow"
    ],

    disabled           : true,
    hidePagingToolbar  : true,
    hideAddButton      : true,
    hideEditButton     : false,
    hideDeleteButton   : true,
    autoReload         : true,
    stateChangeWaitMsg : _("Changing VM state."),

    columns : [{
        header    : _("UUID"),
        hidden    : true,
        dataIndex : "uuid"
    },{
        header    : _("Virtual Machine"),
        flex      : 1,
        sortable  : true,
        dataIndex : "name"
    },{
        header    : "State",
        sortable  : true,
        dataIndex : "state",
        renderer  : function(value, metaData, record) {
            switch(value) {
                case "PoweredOff":
                    return "Powered Off";
                case "LiveSnapshotting":
                    return "Live Snapshotting";
                case "TeleportingPausedVM":
                    return "Teleporting Paused VM";
                case "TeleportingIn":
                    return "Teleporting In";
                case "TakingLiveSnapshot":
                    return "Taking Live Snapshot";
                case "RestoringSnapshot":
                    return "Restoring Snapshot";
                case "DeletingSnapshot":
                    return "Deleting Snapshot";
                case "SettingUp":
                    return "Setting Up";
                default:
                    return value;
            }
        }
    },{
        header    : "Startup Mode",
        sortable  : true,
        dataIndex : "startupMode",
        renderer  : function(value, metaData, record) {
            if(value == "auto") {
                return "Automatic";
            }

            return "Manual";
        }
    }],

    initComponent : function() {
        var me = this;

        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoload   : true,
                remoteSort : false,
                model      : OMV.data.Model.createImplicit({
                    idProperty   : "uuid",
                    totalPoperty : "total",
                    fields       : [
                        { name : "uuid" },
                        { name : "name" },
                        { name : "state" },
                        { name : "startupMode" },
                        { name : "OSTypeId" },
                        { name : "sessionState" }
                    ]
                }),
                proxy : {
                    type    : "rpc",
                    rpcData : {
                        service : "VirtualBox",
                        method  : "getMachines"
                    }
                }
            })
        });

        me.callParent(arguments);
    },

    getTopToolbarItems : function() {
        var me = this;

        var items = me.callParent(arguments);

        Ext.Array.insert(items, 0, [{
            id       : me.getId() + "-start",
            xtype    : "button",
            text     : _("Start"),
            icon     : "images/play.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : me.onStateChangeButton,
            disabled : true,
            scope    : me,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1,
                enabledFn     : me.vmStateButtonEnabled
            },
            action   : "powerUp",
            vmstates : ["PoweredOff", "Paused", "Saved", "Aborted", "Teleported"]
        },{
            id       : me.getId() + "-stop",
            xtype    : "button",
            text     : _("Stop"),
            icon     : "images/shutdown.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled : true,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1,
                enabledFn     : me.vmStateButtonEnabled
            },
            vmstates : ["Running", "Paused", "Stuck"],
            menu     : [{
                id       : me.getId() + "-stop-save-state",
                text     : _("Save the machine state"),
                icon     : "images/save.png",
                iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
                handler  : me.onStateChangeButton,
                scope    : me,
                selectionConfig : {
                    minSelections : 1,
                    maxSelections : 1,
                    enabledFn     : me.vmStateButtonEnabled
                },
                action   : "saveState",
                vmstates : ["Running"]
            },{
                id       : me.getId() + "-stop-power-button",
                text     : _("ACPI Shutdown"),
                icon     : "images/shutdown.png",
                iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
                handler  : me.onStateChangeButton,
                scope    : me,
                selectionConfig : {
                    minSelections : 1,
                    maxSelections : 1,
                    enabledFn     : me.vmStateButtonEnabled
                },
                action   : "powerButton",
                vmstates : ["Running"]
            },{
                id       : me.getId() + "-stop-pause",
                text     : _("Pause"),
                icon     : "images/pause.png",
                iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
                handler  : me.onStateChangeButton,
                scope    : me,
                selectionConfig : {
                    minSelections : 1,
                    maxSelections : 1,
                    enabledFn     : me.vmStateButtonEnabled
                },
                action   : "pause",
                vmstates : ["Running"]
            },{
                id       : me.getId() + "-stop-power-down",
                text     : _("Power off the machine"),
                icon     : "images/shutdown.png",
                iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
                handler  : me.onStateChangeButton,
                scope    : me,
                selectionConfig : {
                    minSelections : 1,
                    maxSelections : 1,
                    enabledFn     : me.vmStateButtonEnabled
                },
                action   : "powerDown",
                vmstates : ["Running", "Paused", "Stuck"]
            },{
                id       : me.getId() + "-stop-reset",
                text     : _("Reset"),
                icon     : "images/reboot.png",
                iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
                handler  : me.onStateChangeButton,
                scope    : me,
                selectionConfig : {
                    minSelections : 1,
                    maxSelections : 1,
                    enabledFn     : me.vmStateButtonEnabled
                },
                action   : "reset",
                vmstates : ["Running"]
            }],
            scope: me
        },{
            xtype : "tbseparator"
        }]);

        Ext.Array.insert(items, items.length, [{
            xtype : "tbseparator"
        },{
            id      : me.getId() + "-phpvirtualbox",
            xtype   : "button",
            text    : _("phpVirtualBox"),
            icon    : "images/virtualbox.png",
            iconCls : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler : function() {
                window.open("/virtualbox/");
            }
        }]);

        return items;
    },

    vmStateButtonEnabled : function(item, records) {
        var state = records[0].get("state");

        if(item.vmstates.indexOf(state) > -1) {
            return true;
        }

        return false;
    },

    /* Handlers */
    onStateChangeButton : function(item, event) {
        var me = this;
        var record = me.getSelected();

        OMV.MessageBox.wait(null, me.stateChangeWaitMsg);
        me.doStateChange(record, item.action);
    },

    onStateChange : function(id, success, response) {
        var me = this;

        if (!success) {
            OMV.MessageBox.hide();
            OMV.MessageBox.error(_("Progress error", response));
        }

        OMV.MessageBox.hide();
        me.doReload();
    },

    doStateChange : function(record, action) {
        var me = this;

        OMV.Rpc.request({
            scope    : me,
            callback : me.onStateChange,
            rpcData  : {
                service : "VirtualBox",
                method  : "setMachineState",
                params  : {
                    uuid  : record.get("uuid"),
                    state : action
                }
            }
        });
    },

    onEditButton : function() {
        var me = this;
        var record = me.getSelected();

        Ext.create("OMV.module.admin.service.virtualbox.MachineEditWindow", {
            uuid         : record.get("uuid"),
            sessionState : record.get("sessionState"),
            startupMode  : record.get("startupMode"),
            listeners    : {
                submit : function() {
                    me.doReload();
                },
                scope   : me
            }
        }).show();
    }

});

OMV.WorkspaceManager.registerPanel({
    id        : "machines",
    path      : "/service/virtualbox",
    text      : _("Virtual Machines"),
    position  : 20,
    className : "OMV.module.admin.service.virtualbox.Machines"
});
