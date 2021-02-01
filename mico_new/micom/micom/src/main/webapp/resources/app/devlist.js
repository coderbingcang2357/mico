var devs = new Vue({
    el: '#devices',
    data: {
        devcount:1,
        pagesize: 20,
        currentPage:0,
        devlist: []
    },
    computed: {
        howmanypages: function () {
                return Math.ceil(this.devcount / this.pagesize);
        },
        pages : function() {
            var pg = [];
            var tmpcpg = this.currentPage;
            var count = 5;
            for (;tmpcpg >= 0 && count > 0; ) {
                pg.unshift(tmpcpg);
                tmpcpg--;
                count--;
            }
            var lastpage = this.howmanypages;
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
            var self = this;
            $.ajax({
              url: "/rest/getdev",
              data: {page:whichpage, pagesize:this.pagesize},
              success: function(d){
                self.currentPage = whichpage;
                self.devcount = d.count;
                self.devlist = d.devs;
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

devs.loadpage(0);
