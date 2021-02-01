var times = 1;
var howmany = 100;
var autoupdate = $('#isautoupdte').prop('checked');;
var timeinter = 1000;// 1s
var server = 1;
function getserverNum() {
    var txt = $("#serverselect").val();
    return parseInt(txt);
}
server = getserverNum();
if (isNaN(server)) {
    server = 1;
}

function timeoutfunc (){
    console.log(server);
    $.ajax({
        url: "/rest/getlog",
        data: {server:server, size:howmany},
        success: function(d){
            if (d.res === "success") {
                var log = d.log;
                log = _.escape(log);
                log = log.replace(/\n/g, "<br />");
                $("#logcontainer").html(log);
            } else {
                $("#logcontainer").html("Request Error !!");
            }
        },
        error: function () {
            $("#logcontainer").html("Request Error !!");
         }
    });
    //var a = "<br/>{}{][]][][][09)(*&^%$#@!>?/]\| ";
    //var log = "";
    //times++;
    //for (var i = 0; i < howmany; ++i) {
    //    log = log + a + times +" "+ i + "\n";
    //}
    //log = _.escape(log);
    //log = log.replace(/\n/g, "<br />");
    //$("#logcontainer").html(log);

    if (autoupdate) {
        setTimeout(timeoutfunc, timeinter);  
    }
}
setTimeout(timeoutfunc, timeinter);
$("#howmanylines").val(String(howmany));
$("#howmanylines").change(function (){
    var txt = $("#howmanylines").val();
    var lines = parseInt(txt);
    if (!isNaN(lines)) {
        var shouldsetval = lines < 1 || lines > 500;
        lines = lines > 500 ? 500 : lines;
        lines = lines < 1 ? 1 : lines;
        if (shouldsetval) {
            $("#howmanylines").val(String(lines));
        }
        howmany = lines;
    }
});

$("#isautoupdte").click(function (){
    autoupdate = $('#isautoupdte').prop('checked');
    if (autoupdate) {
        setTimeout(timeoutfunc, timeinter);
    }
});

$("#serverselect").change(function () {
    var servertmp = getserverNum();
    if (!isNaN(servertmp)) {
        server = servertmp;
    }
});