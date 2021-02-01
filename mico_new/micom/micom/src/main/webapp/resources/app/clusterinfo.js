// clusterinfo 页面操作实现
// 1.设备授权
// 2.添加用户到群
// 群信息, 页面加载时从页面的table中取, 所以这里的代码是和页面息息相关
// 如果页面改了, 这里也要改.
var cluster = {
    id: 0,
    account:"",
    notename:"",
    describ:"",
    creatorid:0,
    ownerid:0,
    createdate:"",
    users:[],
    devs:[]
};

function GetClusterInfo(c)
{
    $("#clusterinfo > tbody > tr").each(function (idx, e) {
        if (idx > 0)
            return;
        $(e).children("td").each(function (i, echild){
            var type = $(echild).data("type");
            if (!type)
                return;
            // "id"
            // "account"
            // "notename"
            // "describ"
            // "creatorid"
            // "ownerid"
            // "createdate"
            var txt = $(echild).text();
            cluster[type]=txt;
        });

    });
}

function GetUsers(c)
{
    $("#userincluster > tbody > tr").each(function (idx, e) {
        var obj = {};
        $(e).children("td").each(function (idx, echild) {
            var type = $(echild).data("type");
            if (!type)
                return;
            // "id"
            // "account"
            // "auth"
            var txt = $(echild).text();
            obj[type] = txt;
        });
        if (obj.id && obj.account && obj.auth)
        {
            cluster.users.push(obj);
        }
    });
}

function GetDevices(c)
{
    $("#devicesincluster > tbody > tr").each(function (idx, e) {
        var obj = {};
        $(e).children("td").each(function (idx, echild) {
            var type = $(echild).data("type");
            if (!type)
                return;
            // "id"
            // "name"
            var txt = $(echild).text();
            obj[type] = txt;
        });
        $(e).find("button").data("row", idx);
        if (obj.id && obj.name) {
            cluster.devs.push(obj);
        }
    });
}

// 把用户显示到设备授权的对话框中
function ShowUsersToDeviceAuthorizeTable(c) 
{
    $("#userAval > tbody").empty();
    c.users.forEach(function (ele, idx) {
        // idx 保存到tr的data-row属性中, 后面可以根据这个row属性知道是哪一个user对象
        var row = '<tr data-row={0}><td><input type="checkbox"></td> <td data-type="id">{1}</td> <td data-type="account">{2}</td> <td data-type="auth">{3}</td> </tr>'
        .format(idx, ele.id, ele.account, ele.auth);
        $("#userAval > tbody").append(row);
    });
}

var currentDev = {};

GetClusterInfo(cluster);
GetUsers(cluster);
GetDevices(cluster);
ShowUsersToDeviceAuthorizeTable(cluster);
// 1.设备授权 =============================================================================================
// 显示设备授权对话框
$(".show-deviceAuthDialog").click(function (e) {
    $("#deviceAuthorize").modal('show');
    // this 是捕获事件的对象button
    var rowofdev = $(this).data("row");
    currentDev = cluster.devs[rowofdev];
    $("#curdevid").text(currentDev.id);
    $("#curdevname").text(currentDev.name);
});

// 提交选择的授权用户
$("#comitAuthorize").click(function () {
    var objselected = [];
    // get row checked
    $("#userAval > tbody > tr").each(function (idx, e) {
        var rownbr = $(e).data("row");
        if (rownbr === undefined)
            return;
        // is checkbox checked
        if ($("input", e).is(':checked')) {
            // ok, 取得被选择的用户
            objselected.push(String(cluster.users[rownbr].id));
        }
    });

    if (objselected.length === 0 || !currentDev.id)
        return;
    // disable submit button
    $("#comitAuthorize").attr("disabled", "true");
    // 把被选择的用户保存ajax发送到服务器进行授权处理
    $.ajax({
        method:"POST",
        url: "/rest/deviceAuthorize",
        data: {
            userid:JSON.stringify(objselected), 
            devid:currentDev.id, 
            adminid:cluster.ownerid},
        success: function(d){
            if (d.res === 'success') {
                ShowInformation("Success", "success", 300, 'auto');
            } else {
                // 失败, 显示失败对话框
                ShowInformation("Error", d.res, 300, 'auto');
            }
            $("#addclusterdlg").modal('hide');
            $("#comitAuthorize").removeAttr("disabled");
        },
        error: function () {
            ShowInformation("Error", "Network error", 300, 'auto');
            $("#addclusterdlg").modal('hide');
            $("#comitAuthorize").removeAttr("disabled");
            // 失败, 显示失败对话框
        }
    });
});

$("#testdiv").click(function (){
    alert("div");
});
$("#testbtn").click(function (){
    alert("button");
});
$("#testspn").click(function (){
    alert("span");
});

// 2.添加用户到群 ==================================================================================
// 首先要搜索到要添加的用户, 这里保存搜到的用户
var searchUserResult = {
    element:"#addUserToCluster_finduserresult",
    users:[],
    setUsers: function (newusers) {
        this.users = newusers;
        this.render();
    },
    rowChecked: function (row) {
        this.users[row].isSelected = !this.users[row].isSelected;
    },
    getSelectedUsers: function () {
        var selecteduser = [];
        for (var i = 0; i < this.users.length; ++i) {
            var user = this.users[i];
            if (user.isSelected) {
                selecteduser.push(user);
            }
        }
        return selecteduser;
    },
    render: function () {
        // 每次render都全部重新生成, 会损失性能, 但是省事
        var toobj = $(this.element + ">tbody");
        toobj.empty();
        for (var i = 0; i < this.users.length; ++i) {
            var user = this.users[i];
            var row = '<tr>\
                        <td><input type="checkbox" {0} onclick="searchUserResult.rowChecked({1})" ></td>\
                        <td>{2}</td>\
                        <td>{3}</td>\
                    </tr>'.format(user.isSelected ? "checked=checked": "",
                        i, // which row
                        user.id,
                        user.account);
            $(row).appendTo(toobj);
        }
    }
};

$("#addUserToCluster_clusterid").text(cluster.id);
$("#addUserToCluster_clustername").text(cluster.account);
// 用户点击搜索时从服务器取数据
$("#adduser_search_user").click(function () {
    // get text
    var searchtext = $("#adduser_searchtext").val();
    if (!searchtext || searchtext === "") {
        console.log("empty search text");
        return;
    }
    // 发请求
    $.ajax({
        method:"POST",
        url: "/rest/commonsearch",
        data: {text:searchtext},
        success: function(d) {
            if (d.res === 'success') {
                // ShowInformation("Success", "success", 300, 'auto');
                if (d.searchres) {
                    var users = [];
                    for (var i = 0; i < d.searchres.length; ++i) {
                        var o = d.searchres[i];
                        // 只要User
                        if (o.type !== "User")
                            continue;
                        users.push({id:o.id, account: o.text, isSelected: false});
                    }
                    searchUserResult.setUsers(users);
                } else {
                    console.log("nothing finded.");
                }
            } else {
                // 失败, 显示失败对话框
                ShowInformation("Error", d.res, 300, 'auto');
            }
        },
        error: function () {
            //var users=[];
            //users.push({id:33, account: "o.text", isSelected: false});
            //users.push({id:34, account: "o.text", isSelected: false});
            //users.push({id:35, account: "o.text", isSelected: false});
            //searchUserResult.setUsers(users);
            ShowInformation("Error", "Network error", 300, 'auto');
            // 失败, 显示失败对话框
        }
    });
});

$("#addUserToCluster_commitAddUser").click(function () {
    var selectusers = searchUserResult.getSelectedUsers();
    console.log(JSON.stringify(selectusers));
    // 发请求
    $.ajax({
        method:"POST",
        url: "/rest/addusertocluster",
        data: {users:JSON.stringify(selectusers), clusterid:cluster.id},
        success: function(d) {
            if (d.res === 'success') {
                ShowInformation("Success", "success", 300, 'auto');
                // add user to cluster.users and append a row to user table
                addSelectedUserToCluserUsers(selectusers);
                addRowToUserTable(selectusers);
            } else {
                // 失败, 显示失败对话框
                ShowInformation("Error", d.res, 300, 'auto');
            }
        },
        error: function () {
            ShowInformation("Error", "Network error", 300, 'auto');
            // 失败, 显示失败对话框
        }
    });
});

function addSelectedUserToCluserUsers(selectuser) {
    for (var i = 0; i < selectuser.length; ++i) {
        var u = selectuser[i];
        cluster.users.push({id:u.id, account:u.account, auth:"Operator"});
    }
    // 更新设备可授权用户表格
    ShowUsersToDeviceAuthorizeTable(cluster);
}

function addRowToUserTable(selectuser) {
    var objto = $("#userincluster > tbody");
    for (var i = 0; i < selectuser.length; ++i) {
        var u = selectuser[i];
        var row = '<tr>\
                    <td data-type="id"><a href="/userinfo/{0}">{1}</a></td>\
                    <td data-type="account">{2}</td>\
                    <td data-type="auth">Operator</td>\
                </tr>'.format(u.id, u.id, u.account);
        $(row).appendTo(objto);
    }
}
