<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<%@ taglib prefix="sec" uri="http://www.springframework.org/security/tags" %>
<%@ page session="true" %>
<%@ page language="java" import="java.util.*" pageEncoding="UTF-8"%>
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <title>MICO Manager</title>
    <script type="text/javascript" src="/resources/app/stringformat.js"></script>
    <!-- Tell the browser to be responsive to screen width -->
    <meta content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" name="viewport">
    <!-- Bootstrap 3.3.6 -->
    <link rel="stylesheet" href="/resources/bootstrap/css/bootstrap.min.css">
    <!-- Font Awesome -->
    <link rel="stylesheet" href="/resources/plugins/font-awesome-4.7.0/css/font-awesome.min.css">
    <!-- Ionicons -->
    <link rel="stylesheet" href="/resources/plugins/ionicons-2.0.1/css/ionicons.min.css">
    <!-- Theme style -->
    <link rel="stylesheet" href="/resources/dist/css/AdminLTE.min.css">
    <!-- AdminLTE Skins. Choose a skin from the css/skins
    folder instead of downloading all of them to reduce the load. -->
    <link rel="stylesheet" href="/resources/dist/css/skins/_all-skins.min.css">
    <!-- iCheck -->
    <link rel="stylesheet" href="/resources/plugins/iCheck/flat/blue.css">
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
      <%@ include file="sidebar.jspf" %>
      <!-- Content Wrapper. Contains page content -->
      <div class="content-wrapper">
        <!-- Main content -->
        <section class="content-header">
          <h1>
          Cluster Information
          <small></small>
          </h1>
        </section>
        <section class="content">
          <!-- Small boxes (Stat box) -->
          <!-- /.row -->
          <!-- Main row -->
          <div class="row">
            <c:forEach items="${clusters}" var="c">
            <!-- Left col -->
            <div class="col-xs-12">
              <div class="nav-tabs-custom">
                <!-- Tabs within a box -->
                <ul class="nav nav-tabs pull-left">
                  <li class="active"><a href="#userinfo-tabb" data-toggle="tab">Information</a></li>
                  <li><a href="#users-tab" data-toggle="tab">Users</a></li>
                  <li><a href="#devices-tab" data-toggle="tab">Devices</a></li>
                </ul>
                <div class="tab-content no-padding">
                  <!-- Morris chart Sales -->
                  <div class="chart tab-pane active" id="userinfo-tabb" style="position: relative;">
                    <table id="clusterinfo" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Account</th>
                          <th>Notename</th>
                          <th>Describ</th>
                          <th>Create ID</th>
                          <th>Owner ID</th>
                          <th>Create Date</th>
                        </tr>
                      </thead>
                      <tbody>
                        <tr>
                          <td data-type="id"><c:out value="${c.id}"/> </td>
                          <td data-type="account"><c:out value="${c.account}" /> </td>
                          <td data-type="notename"><c:out value="${c.notename}" /> </td>
                          <td data-type="describ"><c:out value="${c.describ}" /> </td>
                          <td data-type="creatorid"><a href="/userinfo/${c.creatorID}"><c:out value="${c.creatorID}" /></a> </td>
                          <td data-type="ownerid"><a href="/userinfo/${c.sysAdminID}"><c:out value="${c.sysAdminID}" /></a> </td>
                          <td data-type="createdate"><c:out value="${c.createDate}" /> </td>
                        </tr>
                      </tbody>
                    </table>
                  </div>
                  <!-- users in cluster -->
                  <div class="chart tab-pane" id="users-tab" style="position: relative;">
                    <table id="userincluster" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Account</th>
                          <th>Role</th>
                        </tr>
                      </thead>
                      <tbody>
                        <c:forEach items="${c.users}" var="user">
                        <tr>
                          <td data-type="id"><a href="/userinfo/${user.id}"><c:out value="${user.id}" /></a> </td>
                          <td data-type="account"><c:out value="${user.name}" /> </td>
                          <td data-type="auth"> <c:out value="${user.role}" /> </td>
                        </tr>
                        </c:forEach>
                      </tbody>
                    </table>
                    <div class="btn-group">
                      <button class="btn box-primary glyphicon glyphicon-plus" data-toggle="modal" data-target="#addUserToClusterDialog"></button>
                    </div>
                  </div>
                  <!-- devices in cluster -->
                  <div class="chart tab-pane" id="devices-tab" style="position: relative;">
                    <table id="devicesincluster" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Name</th>
                        </tr>
                      </thead>
                      <tbody>
                        <c:forEach items="${c.devices}" var="dev">
                        <tr>
                          <td data-type="id"> <a href="/devinfo/${dev.id}"><c:out value="${dev.id}"/></a> </td>
                          <td data-type="name"><c:out value="${dev.name}" /></td>
                          <td>
                            <button class="btn show-deviceAuthDialog" data-toggle="modal">
                            <span class="glyphicon glyphicon-user"></span>
                            </button>
                          </td>
                        </tr>
                        </c:forEach>
                      </tbody>
                    </table>
                  </div>
                </div>
              </div>
            </div>
            </c:forEach>
          </div>
              <!-- /.row (main row) -->
        </section>
            <!-- /.content -->
      </div>
          <!-- /.content-wrapper -->
        <%@ include file="foot.jspf" %>
    </div>
    <!-- ./wrapper -->
    <!--设备授权弹出对话框-->
    <div class="modal fade" id="deviceAuthorize" tabindex="-1" role="dialog" aria-labelledby="exampleModalLabel">
      <div class="modal-dialog modal-lg" role="document">
        <div class="modal-content">
          <div class="modal-header">
            <button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>
            <h4 class="modal-title" id="exampleModalLabel">设备授权(Device Authorize)</h4>
          </div>
          <div class="box box-primary">
            <div class="box-header with-border">
              <b>Device ID: </b><span id="curdevid"></span>,
              <b>Device Name: </b><span id="curdevname"></span>
            </div>
            <div class="box-body" style="height: 300px; overflow: auto;">
              <table id="userAval" class="table table-bordered table-hover">
                <thead>
                  <tr>
                    <th></th>
                    <th>ID</th>
                    <th>Account</th>
                    <th>Role</th>
                  </tr>
                </thead>
                <tbody>
                </tbody>
              </table>
            </div>
          </div>
          <div class="modal-footer">
            <button type="button" class="btn btn-default" data-dismiss="modal">Close</button>
            <button id="comitAuthorize" type="button" class="btn btn-primary">Submit</button>
          </div>
        </div>
      </div>
    </div>
    <!-- 添加用户到群弹出对话框-->
    <div class="modal fade" id="addUserToClusterDialog" tabindex="-1" role="dialog" aria-labelledby="exampleModalLabel">
      <div class="modal-dialog modal-lg" role="document">
        <div class="modal-content">
          <div class="modal-header">
            <button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>
            <h4 class="modal-title" id="exampleModalLabel">添加用户(Add User To Cluster)</h4>
          </div>
          <div class="box box-primary">
            <div class="box-header with-border">
              <b>Cluster ID: </b><span id="addUserToCluster_clusterid"></span>,
              <b>Cluster Name: </b><span id="addUserToCluster_clustername"></span>
              <div class="input-group">
                <input id="adduser_searchtext" name="q" class="form-control" placeholder="Search..." type="text">
                  <span class="input-group-btn">
                    <button type="submit" name="search" id="adduser_search_user" class="btn btn-flat"><i class="fa fa-search"></i>
                    </button>
                  </span>
              </div>
            </div>
            <div class="box-body" style="height: 300px; overflow: auto;">
              <table id="addUserToCluster_finduserresult" class="table table-bordered table-hover">
                <thead>
                  <tr>
                    <th></th>
                    <th>ID</th>
                    <th>Account</th>
                  </tr>
                </thead>
                <tbody>
                </tbody>
              </table>
            </div>
          </div>
          <div class="modal-footer">
            <button type="button" class="btn btn-default" data-dismiss="modal">Close</button>
            <button type="button" class="btn btn-primary" id="addUserToCluster_commitAddUser">Submit</button>
          </div>
        </div>
      </div>
    </div>
    <!-- jQuery 2.2.3 -->
    <script src="/resources/plugins/jQuery/jquery-2.2.3.min.js"></script>
    <!-- jQuery UI 1.11.4 -->
    <script src="/resources/plugins/jQueryUI/jquery-ui.min.js"></script>
    <!-- Resolve conflict in jQuery UI tooltip with Bootstrap tooltip -->
    <script>
    $.widget.bridge('uibutton', $.ui.button);
    </script>
    <!-- Bootstrap 3.3.6 -->
    <script src="/resources/bootstrap/js/bootstrap.min.js"></script>
    <!-- Morris.js charts -->
    <!-- Slimscroll -->
    <script src="/resources/plugins/slimScroll/jquery.slimscroll.min.js"></script>
    <!-- FastClick -->
    <script src="/resources/plugins/fastclick/fastclick.js"></script>
    <!-- AdminLTE App -->
    <script src="/resources/dist/js/app.min.js"></script>
    <!-- AdminLTE dashboard demo (This is only for demo purposes) -->
    <script src="/resources/app/clusterinfo.js"></script>
    <script src="/resources/app/infobox.js"></script>
  </body>
</html>