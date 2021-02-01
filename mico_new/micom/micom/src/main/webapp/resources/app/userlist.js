var users = new Vue({
    el: '#users',
    data: {
        usercount:0,
        pagesize: 20,
        currentPage:0,
        userlist: []
    },
    computed: {
        howmanypages: function () {
                return Math.ceil(this.usercount / this.pagesize);
        },
        pages : function() {
            pg = [];
            tmpcpg = this.currentPage;
            count = 5;
            for (;tmpcpg >= 0 && count > 0; ) {
                pg.unshift(tmpcpg);
                tmpcpg--;
                count--;
            }
            lastpage = this.howmanypages;
            for (tmpcpg = this.currentPage+1, count = 4;
                tmpcpg < lastpage && count > 0;
                tmpcpg++, count--) {
                pg.push(tmpcpg);
            }
            return pg;
        }
    },
     methods: {
        loadpage : function (whichpage) {
            //this.userlist = [{id:whichpage, account:'bbb', mail:'aaa@a.a'}];
            self = this;
            $.ajax({
              url: "/rest/getui",
              data: {page:whichpage, pagesize:this.pagesize},
              success: function(d){
                self.currentPage = whichpage;
                self.usercount = d.count;
                self.userlist = d.users;
              },
            });
        },
        gotoPage : function (e) {
            // get input
            // parse to int
            // call load page
            var txt = $(e.target).val();
            var topage = parseInt(txt);
            if (topage >= 0 && topage < this.howmanypages)
            {
                this.loadpage(topage);
            }
        }
     }
});

users.loadpage(0);
