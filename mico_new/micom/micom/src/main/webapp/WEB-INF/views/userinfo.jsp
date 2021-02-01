<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<%@ page session="true" %>
<%@ taglib prefix="sec" uri="http://www.springframework.org/security/tags" %>
<%@ page language="java" import="java.util.*" pageEncoding="UTF-8"%>
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <title>MICO Manager - User Information</title>
    <!-- Tell the browser to be responsive to screen width -->
    <meta content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" name="viewport">
    <script type="text/javascript" src="/resources/vue/vue.js"></script>
    <!-- Bootstrap 3.3.6 -->
    <link rel="stylesheet" href="/resources/bootstrap/css/bootstrap.min.css">
    <!-- Font Awesome -->
    <!-- <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.5.0/css/font-awesome.min.css"> -->
    <link rel="stylesheet" href="/resources/plugins/font-awesome-4.7.0/css/font-awesome.min.css">
    <!-- Ionicons -->
    <!-- <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/ionicons/2.0.1/css/ionicons.min.css"> -->
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
        <section class="content-header">
          <h1>
          User Information
          <small></small>
          </h1>
        </section>
        <!-- Main content -->
        <section class="content">
          <div class="row">
            <c:forEach items="${susers}" var="u">
            <div class="col-xs-12">
              <div class="nav-tabs-custom">
                <ul class="nav nav-tabs pull-left">
                  <li class="active"><a href="#userinfo-tab" data-toggle="tab">Information</a></li>
                  <li><a href="#devices-tab" data-toggle="tab">Devices</a></li>
                  <li><a href="#scenes-tab" data-toggle="tab">Scenes</a></li>
                  <li><a href="#clusters-tab" data-toggle="tab">Clusters</a></li>
                </ul>
                <div class="tab-content no-padding">
                  <div class="chart tab-pane active" id="userinfo-tab" style="position: relative;">
                    <table id="userinfo" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Account</th>
                          <th>Password</th>
                          <th>Mail</th>
                          <th>Signature</th>
                          <th>Head</th>
                          <th>Last Login Date</th>
                          <th>All Online Time</th>
                          <th>Create Date</th>
                        </tr>
                      </thead>
                      <tbody>
                        <tr id="${u.userinfo.id}"><td data-type="id"><a href="/userinfo/${u.userinfo.id}"><c:out value="${u.userinfo.id}"/></a></td>
                        <td><c:out value="${u.userinfo.account}" /> </td>
                        <td data-type="pwd">Click To Modify Password</td>
                        <td data-type="mail"><c:out value="${u.userinfo.mail}" /> </td>
                        <td><c:out value="${u.userinfo.signature}" /> </td>
                        <td><c:out value="${u.userinfo.head}" /> </td>
                        <td><c:out value="${u.userinfo.lastloginTime}" /> </td>
                        <td><c:out value="${u.userinfo.onlinetimes}" /> </td>
                        <td><c:out value="${u.userinfo.createDate}" /> </td></tr>
                      </tbody>
                    </table>
                  </div>
                  <!-- devices -->
                  <div class="chart tab-pane" id="devices-tab" style="position: relative;">
                    <table id="devices" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Name</th>
                          <th>Auth</th>
                          <th>Auth ID</th>
                        </tr>
                      </thead>
                      <tbody>
                        <c:forEach items="${u.devices}" var="d">
                        <tr><td><a href="/devinfo/${d.deviceid}">${d.deviceid}</td>
                        <td><c:out value="${d.deviceName}" /></td>
                        <td> ${d.auth} </td>
                        <td> ${d.authid} </td></tr>
                        </c:forEach>
                      </tbody>
                    </table>
                  </div>
                  <!-- scenes -->
                  <div class="chart tab-pane" id="scenes-tab" style="position: relative;">
                    <table id="scenes" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Auth</th>
                          <th>Note Name</th>
                        </tr>
                      </thead>
                      <tbody>
                        <c:forEach items="${u.scenes}" var="sc">
                        <tr><td><a href="/sceneinfo/${sc.id}">${sc.id}</a></td>
                        <td><c:choose>
                          <c:when test="${sc.role==1}">Read</c:when>
                          <c:when test="${sc.role==2}">Read Write</c:when>
                          </c:choose>
                        </td>
                        <td><c:out value="${sc.notename}"/></td></tr>
                        </c:forEach>
                      </tbody>
                    </table>
                  </div>
                  <!-- clusters -->
                  <div class="chart tab-pane" id="clusters-tab" style="position: relative;">
                    <table id="clusters" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Role</th>
                          <th>Note Name</th>
                        </tr>
                      </thead>
                      <tbody>
                        <c:forEach items="${u.clusters}" var="clu">
                          <tr><td><a href="/clusterinfo/${clu.id}">${clu.id}</a></td>
                          <td><c:choose>
                            <c:when test="${clu.role==1}">Owner</c:when>
                            <c:when test="${clu.role==2}">Manager</c:when>
                            <c:when test="${clu.role==3}">Operator</c:when>
                            </c:choose>
                          </td>
                          <td><c:out value="${clu.notename}"/></td></tr>
                        </c:forEach>
                      </tbody>
                    </table>
                    <div class="btn-group">
                      <button id="showaddclusterdlg" data-userid="${u.userinfo.id}" type="button" class="btn glyphicon glyphicon-plus" data-toggle="modal" data-target="#addclusterdlg"></button>
                    </div>
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
    <div class="modal fade bs-example-modal-sm" tabindex="-1" role="dialog" aria-labelledby="mySmallModalLabel" id="modifyuserPwdDialog" data-backdrop="static">
      <div class="modal-dialog modal-sm" role="document">
        <div class="modal-content">
          <div class="modal-header">
            <button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>
            <h4 class="modal-title" id="exampleModalLabel">Modify Password</h4>
          </div>
          <div class="box box-info">
            <form class="form-horizontal">
              <div class="box-body">
                <div class="form-group">
                  <label for="inputEmail3" class="col-sm-3 control-label">Password</label>
                  <div class="col-sm-9">
                    <input v-model="pwd" class="form-control" id="inputEmail3" placeholder="password" type="text">
                  </div>
                </div>
                <div class="form-group">
                  <div class="col-sm-offset-2 col-sm-10">
                    <button type="button" v-bind:disabled="!enablecommit" v-on:click="commitNewPwd" class="btn btn-info pull-right">Submit</button>
                  </div>
                </div>
              </div>
            </form>
          </div>
        </div>
      </div>
    </div>
    <div class="modal fade bs-example-modal-sm" tabindex="-1" role="dialog" aria-labelledby="mySmallModalLabel" id="modifyuserMailDialog" data-backdrop="static">
      <div class="modal-dialog modal-sm" role="document">
        <div class="modal-content">
          <div class="modal-header">
            <button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>
            <h4 class="modal-title" id="exampleModalLabel">Modify Mail</h4>
          </div>
          <div class="box box-info">
            <form class="form-horizontal">
              <div class="box-body">
                <div class="form-group">
                  <label for="inputEmail3" class="col-sm-2 control-label">Email</label>
                  <div class="col-sm-10">
                    <input v-model="mail" class="form-control" id="inputEmail3" placeholder="Email" type="email">
                  </div>
                </div>
                <div class="form-group">
                  <div class="col-sm-offset-2 col-sm-10">
                    <button vtype="button" v-bind:disabled="!enablecommit" v-on:click="commitNewMail" class="btn btn-info pull-right">Submit</button>
                  </div>
                </div>
              </div>
            </form>
          </div>
        </div>
      </div>
    </div>

     <!--add new cluster dialog -->
    <div class="modal fade" id="addclusterdlg" tabindex="-1" role="dialog" aria-labelledby="exampleModalLabel">
      <div class="modal-dialog" role="document">
        <div class="modal-content">
          <div class="modal-header">
            <button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>
            <h4 class="modal-title" id="exampleModalLabel">New Cluster</h4>
          </div>
          <div class="modal-body">
            <form>
              <div class="form-group">
                <label for="recipient-name" class="control-label">Name:</label>
                <input type="text" class="form-control" id="newcluster-name">
              </div>
              <div class="form-group">
                <label for="message-text" class="control-label">Description:</label>
                <textarea class="form-control" id="newcluster-desc"></textarea>
              </div>
            </form>
          </div>
          <div class="modal-footer">
            <button type="button" class="btn btn-default" data-dismiss="modal">Close</button>
            <button type="button" class="btn btn-primary" id="submit-create-newcluster">Submit</button>
          </div>
        </div>
      </div>
    </div>

    <!-- ./wrapper -->
    <!-- jQuery 2.2.3 -->
    <script src="/resources/plugins/jQuery/jquery-2.2.3.min.js"></script>
    <!-- jQuery UI 1.11.4 -->
    <!-- <script src="https://code.jquery.com/ui/1.11.4/jquery-ui.min.js"></script> -->
    <script src="/resources/plugins/jQueryUI/jquery-ui.min.js"></script>
    <!-- Resolve conflict in jQuery UI tooltip with Bootstrap tooltip -->
    <script>
    $.widget.bridge('uibutton', $.ui.button);
    </script>
    <!-- Bootstrap 3.3.6 -->
    <script src="/resources/bootstrap/js/bootstrap.min.js"></script>
    <!-- Morris.js charts -->
    <!-- <script src="https://cdnjs.cloudflare.com/ajax/libs/raphael/2.1.0/raphael-min.js"></script> -->
    <!-- Slimscroll -->
    <script src="/resources/plugins/slimScroll/jquery.slimscroll.min.js"></script>
    <!-- FastClick -->
    <script src="/resources/plugins/fastclick/fastclick.js"></script>
    <!-- AdminLTE App -->
    <script src="/resources/dist/js/app.min.js"></script>
    <!-- AdminLTE dashboard demo (This is only for demo purposes) -->
    <script src="/resources/dist/js/pages/dashboard.js"></script>
    <!-- AdminLTE for demo purposes -->
    <script src="/resources/dist/js/demo.js"></script>
    <script src="/resources/app/modifyuserinfo.js"></script>
    <script src="/resources/app/addcluster.js"></script>
  </body>
</html>