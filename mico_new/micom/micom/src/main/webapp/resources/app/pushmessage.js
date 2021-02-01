// message: {"id":123, "message":"this is a message", "createdate":"2013-08-08 11:11:11", "shouldsend":0}
var pushmessages = new Vue({
    el: '#pushmessages',
    data: {
        messages: [
                {"id":123, "message":"this is a message", "createdate":"2013-08-08 11:11:11","shouldsend":0}, 
                {"id":124, "message":"this is a message", "createdate":"2013-08-08 11:11:11","shouldsend":0}, 
                {"id":125, "message":"this is a message", "createdate":"2013-08-08 11:11:11","shouldsend":1}]
    },
    methods: {
        deletemessage : function (idx) {

            //console.log("delete msg:" + this.messages[idx].id);
            //this.messages.splice(idx, 1);
            deletePushMessage(this, this.messages[idx].id);
        },
        addnewmessage : function () {
            console.log("add new message");
        },
        sendpushmessage : function (idx) {
            notifyServerSendPushMessage(this, this.messages[idx].id);
        },
        deletePushMessageByID: function (msgid) {
            var idx = this.findMessageIndex(msgid);
            if (idx >= 0) {
                this.messages.splice(idx, 1);
            }
        },
        tagPushMessageSended: function (msgid) {
            var idx = this.findMessageIndex(msgid);
            if (idx >= 0) {
                this.messages[idx].shouldsend = 1;
            }
        },
        findMessageIndex: function (msgid) {
            for (var idx = 0; idx < this.messages.length; ++idx) {
                if (this.messages[idx].id === msgid) {
                    return idx;
                }
            }
            return -1;
        }
    }
});

var id = 10;
$("#addnewpm").click(function(){
     $("textarea#inputpushmessage").val("");
});

// pushmessage-insert message:
$("#addnewpushmessage_commit").click(function(){
    $("#addnewpushmessage_commit").prop("disabled", true);
    var msg = $("textarea#inputpushmessage").val();
    //++id;
    var now = dateToString(new Date());
    //console.log({"id":id, "message":msg + id, "createdate":now});
    //pushmessages.messages.push({"id":id, "message":msg + id, "createdate":now});
    //$("#addnewpushmessage").modal('hide');
    $.ajax({
        method:"POST",
        url: "/pushmessage-insert",
        data: {message:msg},
        success: function(d) {
            if (d.res === 'success') {
                pushmessages.messages.push({"id":d.id, "message":msg, "createdate":now, "shouldsend":0});
                ShowInformation("Success", "Insert push message success", 300, 'auto');
            } else {
                // 失败, 显示失败对话框
                ShowInformation("Failed", "insert push message failed.1", 300, 'auto');
            }
            //$("#addnewpushmessage").modal('hide');
            $("#addnewpushmessage_commit").removeAttr("disabled");
        },
        error: function () {
            //ShowInformation("Error", "Network error", 300, 'auto');
            //alert("insert push message failed.");
            ShowInformation("Failed", "insert push message failed", 300, 'auto');
            //$("#addnewpushmessage").modal('hide');
            $("#addnewpushmessage_commit").removeAttr("disabled");
            // 失败, 显示失败对话框
        }
    });
});

// pushmessage-del    id
function deletePushMessage(vueobj, msgid) {
    $("#delete-" + msgid.toString()).hide();
    $("#delete-busing-" + msgid.toString()).show();

    $.ajax({
        method:"POST",
        url: "/pushmessage-del",
        data: {id:msgid},
        success: function(d) {
            $("#delete-" + msgid.toString()).show();
            $("#delete-busing-" + msgid.toString()).hide();
            if (d.res === 'success') {
                //vueobj.messages.splice(idx, 1);
                //
                vueobj.deletePushMessageByID(msgid);
                ShowInformation("Success", "delete push message success", 300, 'auto');
            } else {
                // 失败, 显示失败对话框
                ShowInformation("Failed", "delete push message failed.1", 300, 'auto'); 
            }
        },
        error: function () {
            setTimeout(function(){
                ShowInformation("Failed", "delete push message failed", 300, 'auto');
                $("#delete-" + msgid.toString()).show();
                $("#delete-busing-" + msgid.toString()).hide();
            }, 2000)

        }
    });
}

// pushmessage-getall
(function loadAllPushMessage() {
    $.ajax({
        method:"POST",
        url: "/pushmessage-getall",
        //data: {id:msgid},
        success: function(d) {
            if (d.res === 'success') {
                pushmessages.messages = d.messages;
            } else {
                // 失败, 显示失败对话框
                ShowInformation("Failed", "Get push message failed.1", 300, 'auto');
            }
        },
        error: function () {
            ShowInformation("Failed", "Get push message failed", 300, 'auto');
        }
    });
})();

function notifyServerSendPushMessage(vueobj, msgid){
    $("#send-" + msgid.toString()).hide();
    $("#send-busing-" + msgid.toString()).show();
    $.ajax({
        method:"POST",
        url: "/pushmessage-send",
        data: {id:msgid},
        success: function(d) {
            if (d.res === 'success') {
                vueobj.tagPushMessageSended(msgid);
                //$("#send-busing-"+string(msgid)).hide();
            } else {vueobj.tagPushMessageSended(msgid);
                // 失败, 显示失败对话框
                ShowInformation("Failed", "Send push message failed.1", 300, 'auto');
            }
            $("#send-" + msgid.toString()).show();
            $("#send-busing-" + msgid.toString()).hide();
        },
        error: function () {
            //vueobj.tagPushMessageSended(msgid);
            setTimeout(function() {
                ShowInformation("Failed", "Send push message failed 2", 300, 'auto');
                $("#send-" + msgid.toString()).show();
                $("#send-busing-" + msgid.toString()).hide();
            }, 2000);
        }
    });
}

function dateToString(adate) {
    var strdate = adate.getFullYear() + "-" + adate.getMonth() + "-" + adate.getDay()
    + " " + adate.getHours() + ":" + adate.getMinutes() + ":" + adate.getSeconds()
    return strdate;
}
