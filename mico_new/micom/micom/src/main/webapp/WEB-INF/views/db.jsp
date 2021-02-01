<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<%@ page session="true" %>
<%@ taglib prefix="sec" uri="http://www.springframework.org/security/tags" %>
<%@ page language="java" import="java.util.*" pageEncoding="UTF-8"%>
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <title>MICO Manager</title>
    <!-- Tell the browser to be responsive to screen width -->
    <meta content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" name="viewport">
    <script type="text/javascript" src="./resources/vue/vue.js"></script>
    <!-- Bootstrap 3.3.6 -->
    <link rel="stylesheet" href="./resources/bootstrap/css/bootstrap.min.css">
    <!-- Font Awesome -->
    <!-- <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.5.0/css/font-awesome.min.css"> -->
    <link rel="stylesheet" href="./resources/plugins/font-awesome-4.7.0/css/font-awesome.min.css">
    <!-- Ionicons -->
    <!-- <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/ionicons/2.0.1/css/ionicons.min.css"> -->
    <link rel="stylesheet" href="./resources/plugins/ionicons-2.0.1/css/ionicons.min.css">
    <!-- Theme style -->
    <link rel="stylesheet" href="./resources/dist/css/AdminLTE.min.css">
    <!-- AdminLTE Skins. Choose a skin from the css/skins
    folder instead of downloading all of them to reduce the load. -->
    <link rel="stylesheet" href="./resources/dist/css/skins/_all-skins.min.css">
    <!-- iCheck -->
    <link rel="stylesheet" href="./resources/plugins/iCheck/flat/blue.css">
    <!-- HTML5 Shim and Respond.js IE8 support of HTML5 elements and media queries -->
    <!-- WARNING: Respond.js doesn't work if you view the page via file:// -->
    <!--[if lt IE 9]>
    <script src="https://oss.maxcdn.com/html5shiv/3.7.3/html5shiv.min.js"></script>
    <script src="https://oss.maxcdn.com/respond/1.4.2/respond.min.js"></script>
    <![endif]-->
  </head>
  <body class="hold-transition skin-blue sidebar-mini fixed">
    <div class="wrapper">
      <%@ include file="header.jspf" %>
      <!-- Left side column. contains the logo and sidebar -->
      <aside class="main-sidebar">
        <!-- sidebar: style can be found in sidebar.less -->
        <section class="sidebar">
          <!-- Sidebar user panel -->
          <!-- sidebar menu: : style can be found in sidebar.less -->
          <ul class="sidebar-menu">
            <li>
              <a href="/searchuser">
                <i class="fa fa-files-o"></i>
                <span>Search</span>
              </a>
            </li>
            <li>
              <a href="/online">
                <i class="fa fa-th"></i> <span>Online</span>
                <span class="pull-right-container">
                  <!-- <small class="label pull-right bg-green">new</small> -->
                </span>
              </a>
            </li>

            <li>
              <a href="/serverlist">
                <i class="fa fa-laptop"></i>
                <span>Server List</span>
                <span class="pull-right-container">
                  <i class="fa fa-angle-left pull-right"></i>
                </span>
              </a>
            </li>
            <li class="treeview active">
              <a href="#">
                <i class="fa fa-laptop"></i>
                <span>Database</span>
                <span class="pull-right-container">
                  <i class="fa fa-angle-left pull-right"></i>
                </span>
              </a>
               <ul class="treeview-menu" style="display: block;">
                <li><a href="#" id="showusers"><i class="fa fa-circle-o"></i> Users</a></li>
                <li><a href="#" id="showdev"><i class="fa fa-circle-o"></i> Devices</a></li>
                <li><a href="#" id="showcluster"><i class="fa fa-circle-o"></i> Clusters</a></li>
              </ul>
            </li>

            <li>
              <a href="/pushmessage">
                <i class="fa fa-laptop"></i>
                <span>Push Messages</span>
                </span>
              </a>
            </li>

            <li>
              <a href="/feedback">
                <i class="fa fa-laptop"></i>
                <span>FeedBack</span>
                <span class="pull-right-container">
                  <i class="fa fa-angle-left pull-right"></i>
                </span>
              </a>
            </li>
          </ul>
        </section>
      </aside>

      <!-- Content Wrapper. Contains page content -->
      <div class="content-wrapper" style="min-height: 900px;">
        <!-- Main content -->
        <section class="content" id="usersection">
          <!-- Small boxes (Stat box) -->
          <!-- /.row -->
          <!-- Main row -->
          <div class="row">
            <!-- Left col -->
            <div class="col-xs-12" id="users">
              <h1>
              User Count:
              <small>{{usercount}}</small>
              </h1>
              <div class="box box-solid">
                <div class="box-body">
                  <form class="form-inline">
                    <div class="input-group">
                      <label for="usercurpage" class="input-group-addon">Go To Page</label>
                      <input id="usercurpage" type="text" class="form-control" v-on:change="gotoPage($event)" placeholder="" min="0" v-bind:max="howmanypages">
                    </div>
                    <div class="input-group">
                      <label for="userpagesize" class="input-group-addon">Page Size</label>
                      <input type="text" class="form-control" id="userpagesize" placeholder="" min="10" max="1000" v-model="pagesize">
                    </div>
                  </form>
                </div>
              </div>
              <div class="nav-tabs-custom">
                <table id="users-tb" class="table table-bordered table-hover">
                  <thead>
                    <tr>
                      <th>ID</th>
                      <th>Name</th>
                      <th>Mail</th>
                      <th>Create Date</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr v-for="item in userlist">
                      <td><a v-bind:href="'/userinfo/'+item.id" >{{item.id}}</a></td>
                      <td>{{item.account}}</td>
                      <td>{{item.mail}}</td>
                      <td>{{item.createDateStr}}</td>
                    </tr>
                  </tbody>
                </table>
                <div class="btn-group">
                  <button class="btn btn-default" v-for="item in pages" v-on:click="loadpage(item)">
                  <span v-if="item == currentPage" style="font-weight:bold;">{{item}}</span>
                  <span v-else>{{item}}</span>
                  </button>
                  <button class="btn btn-default">All pages: {{howmanypages}}</button>
                </div>
              </div>
            </div>
          </div>
          <!-- /.row (main row) -->
        </section>
        <section class="content" id="devsection">
          <div class="row">
            <!-- Left col -->
            <div class="col-xs-12" id="devices">
              <h1>
              Device Count: <small>{{devcount}}</small>
              </h1>
              <div class="box box-solid">
                <div class="box-body">
                  <form class="form-inline">
                    <div class="input-group">
                      <label for="usercurpage" class="input-group-addon">Go To Page</label>
                      <input id="usercurpage" type="text" class="form-control" v-on:change="gotoPage($event)" placeholder="" min="0" v-bind:max="howmanypages">
                    </div>
                    <div class="input-group">
                      <label for="userpagesize" class="input-group-addon">Page Size</label>
                      <input type="text" class="form-control" id="userpagesize" placeholder="" min="10" max="1000" v-model="pagesize">
                    </div>
                  </form>
                </div>
              </div>
              <div class="box box-solid">
                <table id="users-tb" class="table table-bordered table-hover">
                  <thead>
                    <tr>
                      <th>ID</th>
                      <th>Firm ID</th>
                      <th>Name</th>
                      <th>Mac</th>
                      <th>Cluster ID</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr v-for="item in devlist">
                      <td><a v-bind:href="'/devinfo/'+item.id_str" >{{item.id_str}}</a></td>
                      <td>{{item.firmClusterID}}</td>
                      <td>{{item.name}}</td>
                      <td>{{item.macAddr}}</td>
                      <td><a v-bind:href="'/clusterinfo/'+item.clusterID">{{item.clusterID}}</a></td>
                    </tr>
                  </tbody>
                </table>
                <div class="btn-group">
                  <button class="btn btn-default" v-for="item in pages" v-on:click="loadpage(item)">
                  <span v-if="item == currentPage" style="font-weight:bold;">{{item}}</span>
                  <span v-else>{{item}}</span>
                  </button>
                  <button class="btn btn-default">All pages: {{howmanypages}}</button>
                </div>
              </div>
            </div>
          </div>
        </section>
        <section class="content" id="clustersection">
          <div class="row">
            <!-- Left col -->
            <div class="col-xs-12" id="clusters">
              <h1>
              Cluster Count: <small>{{clustercount}}</small>
              </h1>
              <div class="box box-solid">
                <div class="box-body">
                  <form class="form-inline">
                    <div class="input-group">
                      <label for="usercurpage" class="input-group-addon">Go To Page</label>
                      <input id="usercurpage" type="text" class="form-control" v-on:change="gotoPage($event)" placeholder="" min="0" v-bind:max="howmanypages">
                    </div>
                    <div class="input-group">
                      <label for="userpagesize" class="input-group-addon">Page Size</label>
                      <input type="text" class="form-control" id="userpagesize" placeholder="" min="10" max="1000" v-model="pagesize">
                    </div>
                  </form>
                </div>
              </div>
              <div class="box box-solid">
                <table id="users-tb" class="table table-bordered table-hover">
                  <thead>
                    <tr>
                      <th>ID</th>
                      <th>Account</th>
                      <th>Creator ID</th>
                      <th>SysAdmin ID</th>
                      <th>Create Date</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr v-for="item in clusterlist">
                      <td><a v-bind:href="'/clusterinfo/'+item.id_str" >{{item.id_str}}</a></td>
                      <td>{{item.account}}</td>
                      <td><a v-bind:href="'/userinfo/'+item.creatorID">{{item.creatorID}}</a></td>
                      <td><a v-bind:href="'/userinfo/'+item.sysAdminID">{{item.sysAdminID}}</a></td>
                      <td>{{item.createDate}}</td>
                    </tr>
                  </tbody>
                </table>
                <div class="btn-group">
                  <button class="btn btn-default" v-for="item in pages" v-on:click="loadpage(item)">
                  <span v-if="item == currentPage" style="font-weight:bold;">{{item}}</span>
                  <span v-else>{{item}}</span>
                  </button>
                  <button class="btn btn-default">All pages: {{howmanypages}}</button>
                </div>
              </div>
            </div>
          </div>
        </section>
        <!-- /.content -->
      </div>
      <!-- /.content-wrapper -->
      <%@ include file="foot.jspf" %>
    </div>
    <!-- ./wrapper -->
    <!-- jQuery 2.2.3 -->
    <script src="./resources/plugins/jQuery/jquery-2.2.3.min.js"></script>
    <!-- jQuery UI 1.11.4 -->
    <!-- <script src="https://code.jquery.com/ui/1.11.4/jquery-ui.min.js"></script> -->
    <script src="./resources/plugins/jQueryUI/jquery-ui.min.js"></script>
    <!-- Resolve conflict in jQuery UI tooltip with Bootstrap tooltip -->
    <script>
    $.widget.bridge('uibutton', $.ui.button);
    </script>
    <!-- Bootstrap 3.3.6 -->
    <script src="./resources/bootstrap/js/bootstrap.min.js"></script>
    <!-- Morris.js charts -->
    <!-- <script src="https://cdnjs.cloudflare.com/ajax/libs/raphael/2.1.0/raphael-min.js"></script> -->
    <!-- Slimscroll -->
    <script src="./resources/plugins/slimScroll/jquery.slimscroll.min.js"></script>
    <!-- FastClick -->
    <script src="./resources/plugins/fastclick/fastclick.js"></script>
    <!-- AdminLTE App -->
    <script src="./resources/dist/js/app.min.js"></script>
    <!-- AdminLTE dashboard demo (This is only for demo purposes) -->
    <script src="./resources/dist/js/pages/dashboard.js"></script>
    <!-- AdminLTE for demo purposes -->
    <script src="./resources/dist/js/demo.js"></script>
    <script type="text/javascript" src="./resources/app/userlist.js"></script>
    <script type="text/javascript" src="./resources/app/devlist.js"></script>
    <script type="text/javascript" src="./resources/app/clusterlist.js"></script>
    <script type="text/javascript" src="./resources/app/stack.js"></script>
    <script type="text/javascript">
    var sm = new StackManager(['#usersection', '#devsection', '#clustersection']);
    sm.setCurrent('#usersection');
    $("#showusers").click(function(){
    sm.setCurrent("#usersection");
    $("#showusers").addClass('active');
    $("#showdev").removeClass('active');
    $("#showcluster").removeClass('active');
    });
    $("#showdev").click(function(){
    sm.setCurrent("#devsection");
    $("#showusers").removeClass('active');
    $("#showdev").addClass('active');
    $("#showcluster").removeClass('active');
    });
    $("#showcluster").click(function(){
    sm.setCurrent("#clustersection");
    $("#showusers").removeClass('active');
    $("#showdev").removeClass('active');
    $("#showcluster").addClass('active');
    });
    </script>
  </body>
</html>