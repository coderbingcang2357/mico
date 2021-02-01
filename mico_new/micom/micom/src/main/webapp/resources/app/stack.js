// 保存一些元素的id, 通过setCurrent来设定显示哪一个元素
function StackManager(idarr) {
    'user strict'
    this.idlist = idarr;
}

StackManager.prototype.setCurrent = function(id) {
    this.idlist.forEach(function (idi, index) {
        t = $(idi);
        if (id === idi) {
            t.fadeIn();       
        } else { 
            t.hide();
        }
    });
}
