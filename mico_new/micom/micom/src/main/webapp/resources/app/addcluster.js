String.prototype.format = function(args) 
{ 
    if (arguments.length>0) 
    { 
        var result = this; 
        if (arguments.length == 1 && typeof (args) == "object")
        { 
            for (var key in args)
            { 
                var reg=new RegExp ("({"+key+"})","g"); 
                result = result.replace(reg, args[key]); 
            } 
        } 
        else 
        { 
            for (var i = 0; i < arguments.length; i++) 
            { 
                if(arguments[i]==undefined) 
                { 
                    return ""; 
                } 
                else 
                { 
                    var reg=new RegExp ("({["+i+"]})","g"); 
                    result = result.replace(reg, arguments[i]); 
                } 
            } 
        } 
        return result; 
    } 
    else 
    { 
        return this; 
    } 
} 

function InsertRowToTable(clusterid, name, desc) {
    var row = '<tr><td><a href="/clusterinfo/{0}">{1}</a></td><td>Owner</td><td>{2},{3}</td></tr>'.format(clusterid, clusterid, name, desc);
    $("#clusters > tbody").append(row);
}

$("#showaddclusterdlg").click(function() {
    $("#newcluster-name").val("");
    $("#newcluster-desc").val("");
});

$("#submit-create-newcluster").click(function() {
    $("#submit-create-newcluster").attr("disabled", "true");
    var userid = $("#showaddclusterdlg").data("userid");
    var name = $("#newcluster-name").val();
    var desc = $("#newcluster-desc").val();
    $.ajax({
        method:"POST",
        url: "/rest/createcluster",
        data: {
            userid:userid, 
            name:name, 
            desc:desc},
            success: function(d){
            if (d.res === 'success') {
                InsertRowToTable(d.clusterid, name, desc);
            }
            $("#addclusterdlg").modal('hide');
            $("#submit-create-newcluster").removeAttr("disabled");
        },
        error: function () {
                $("#addclusterdlg").modal('hide');
                $("#submit-create-newcluster").removeAttr("disabled");
            }
    });
});