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
            if(value == "auto")
                return "Automatic";
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
                    idProperty : "uuid",
                    totalPoperty : "total",
                    fields : [
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
            action   : "powerUp"
        },{
            id       : me.getId() + "-stop",
            xtype    : "button",
            text     : _("Stop"),
            icon     : "images/shutdown.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            disabled : true,
            menu     : [{
                id       : me.getId() + "-stop-saveState",
                vmstates : ["Running"],
                text     : _("Save the machine state"),
                icon     : "/virtualbox/images/vbox/fd_16px.png",
                handler  : me.onStateChangeButton,
                scope    : me,
                action   : "saveState"
            },{
                id       : me.getId() + "-stop-powerButton",
                vmstates : ["Running"],
                text     : _("ACPI Shutdown"),
                icon     : "/virtualbox/images/vbox/acpi_16px.png",
                handler  : me.onStateChangeButton,
                scope    : me,
                action   : "powerButton"
            },{
                id       : me.getId() + "-stop-pause",
                vmstates : ["Running"],
                text     : _("Pause"),
                icon     : "/virtualbox/images/vbox/pause_16px.png",
                handler  : me.onStateChangeButton,
                scope    : me,
                action   : "pause"
            },{
                id       : me.getId() + "-stop-powerDown",
                vmstates : ["Running", "Paused", "Stuck"],
                text     : _("Power off the machine"),
                icon     : "/virtualbox/images/vbox/poweroff_16px.png",
                handler  : me.onStateChangeButton,
                scope    : me,
                action   : "powerDown"
            },{
                id       : me.getId() + "-stop-reset",
                vmstates : ["Running"],
                text     : _("Reset"),
                icon     : "/virtualbox/images/vbox/reset_16px.png",
                handler  : me.onStateChangeButton,
                scope    : me,
                action   : "reset"
            }],
            scope: me
        },{
            xtype : "tbseparator"
        }]);

        Ext.Array.insert(items, items.length, [{
            xtype : "tbseparator"
        },{
            id      : me.getId() + "-phpvbx",
            xtype   : "button",
            text    : _("phpVirtualBox"),
            icon    : "/virtualbox/images/vbox/OSE/VirtualBox_16px.png",
            handler : function() {
                window.open("/virtualbox/");
            },
            scope: me
        }]);

        return items;
    },

    doReload : function() {
       var me = this;
       me.store.reload();
       me.changeButtonStatus(me.getSelection());
    },

    onSelectionChange : function(model, records) {
        var me = this;
        me.callParent(arguments);
        me.changeButtonStatus(records);
    },

    changeButtonStatus : function(records) {
        var me = this,
            i;

        var tbarBtnName = [
            "start",
            "stop",
            "edit"
        ];

        var tbarBtnEnabled = {
            "start" : false,
            "stop" : false,
            "edit" : false
        };

        if (records.length === 1) {

            var record = me.getSelected();
            var state = record.get("state");

            if(["PoweredOff", "Paused", "Saved", "Aborted", "Teleported"].indexOf(state) > -1) {
                tbarBtnEnabled["start"] = true;
            } else {
                tbarBtnEnabled["start"] = false;
            }

            if(["Running", "Paused", "Stuck"].indexOf(state) > -1) {
                tbarBtnEnabled["stop"] = true;
            } else {
                tbarBtnEnabled["stop"] = false;
            }

            tbarBtnEnabled["edit"] = true;

            var stopButton = me.queryById(me.getId() + "-" + "stop");

            var menu = stopButton.menu.items;
            for(i = 0; i < menu.items.length; i++) {
                if(menu.items[i].vmstates.indexOf(state) > -1) {
                    menu.items[i].enable();
                } else {
                    menu.items[i].disable();
                }
            }
        }

        for (i = 0, j = tbarBtnName.length; i < j; i++) {
            var tbarBtnCtrl = me.queryById(me.getId() + "-" +
                tbarBtnName[i]);

            if (!Ext.isEmpty(tbarBtnCtrl)) {
                if (tbarBtnEnabled[tbarBtnName[i]] === false) {
                    tbarBtnCtrl.disable();
                } else {
                    tbarBtnCtrl.enable();
                }
            }
        }
    },

    /* Handlers */
    onStateChangeButton : function(item, event) {
        var me = this;
        var record = me.getSelected();

        OMV.MessageBox.wait(null, me.stateChangeWaitMsg);
        console.log(item);
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
