// 
var modifymailvue = new Vue ({
    el: "#modifyuserPwdDialog",
    data: {
        id:"",
        pwd:"",
        mail:"",
        enablecommit: true
    },
    methods: {
        commitNewPwd : function () {
            self = this;
            self.enablecommit = false;
            $.ajax({
              method:"POST",
              url: "/rest/modifyuserpassword",
              data: {id:this.id, newpwd:this.pwd},
              success: function(d){
                if (d.res === 'success') {
                    // modify table content
                    modifyMailTextInTable(self.id, "mail", self.mail);
                    self.enablecommit = true;
                    $("#modifyuserPwdDialog").modal('hide');
                }
                else
                {
                    alert("modify password failed: " + self.id+ ":" + self.pwd);
                }
              },
              error: function () {
                self.enablecommit = true;
                alert("modify password failed: " + self.id+ ":" + self.pwd);
                $("#modifyuserPwdDialog").modal('hide');
              }
            });
        }
    }
});

function modifyMailTextInTable(rowid, columnname, newtext) {
    // javascript will auto convert rowid to String
    $("tr#"+rowid + "> td").each(function (idx, e) {
         var type = $(e).data('type');
         if (type === columnname) {
            $(e).text(newtext);
         }
    });
}
// 
var modifypwdvue = new Vue ({
    el: "#modifyuserMailDialog",
    data: {
        id:"",
        pwd:"",
        mail:"",
        enablecommit: true
    },
    methods: {
        commitNewMail : function () {
            self = this;
            self.enablecommit = false;
            console.log(self.enablecommit);
            $.ajax({
              method:"POST",
              url: "/rest/modifyusermail",
              data: {id:this.id, newmail:this.mail},
              success: function(d){
                if (d.res === 'success') {
                    // modify table content
                    modifyMailTextInTable(self.id, "mail", self.mail);
                    self.enablecommit = true;
                    $("#modifyuserMailDialog").modal('hide');
                }
              },
              error: function () {
                self.enablecommit = true;
                $("#modifyuserMailDialog").modal('hide');
              }
            });
        }
    }
});

$("table#userinfo tr").click(function(e){
    var jqtarget = $(e.target); // e.target is "td" element
    // the type of "td" is set via "data-type=pwd" in html
    var tdtype = jqtarget.data("type");
    if (!tdtype)
        return;
    // keyword this refers to the DOM element to which the handler is bound
    // that is  table#userinfo tr 
    $("td", this).each(function(idx, e) {
        // "this" is dom element "td"
        var tdtype = $(this).data("type");
        if (tdtype === "id") {
            modifymailvue.id = $(this).text();
            modifypwdvue.id = modifymailvue.id;
        }
        else if (tdtype === "pwd") {
            //modifymailvue.pwd = $(this).text();
            //modifypwdvue.pwd = modifymailvue.pwd;
        }
        else if (tdtype === "mail") {
            modifymailvue.mail = $(this).text();
            modifypwdvue.mail = modifymailvue.mail;
        }
    });
    // show dialog to modify something
    if (tdtype === "pwd") {
        $("#modifyuserPwdDialog").modal('show');  
    } else if (tdtype === "mail") {
        $("#modifyuserMailDialog").modal('show');       
    }
});
