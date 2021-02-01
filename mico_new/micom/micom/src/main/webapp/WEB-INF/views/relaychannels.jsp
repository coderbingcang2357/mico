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
    <!-- Tell the browser to be responsive to screen width -->
    <meta content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" name="viewport">
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
    <link rel="stylesheet" href="/resources/plugins/datatables/jquery.dataTables.css">
  </head>
  <body class="hold-transition skin-blue sidebar-mini fixed">
    <div class="wrapper">
      <%@ include file="header.jspf" %>
      <!-- Left side column. contains the logo and sidebar -->
      <%@ include file="sidebar.jspf" %>
      <!-- Content Wrapper. Contains page content -->
      <div class="content-wrapper">
        <!-- Content Header (Page header) -->
        <!--
        <section class="content-header">
          <h1>
          Dashboard
          <small>Control panel</small>
          </h1>
          <ol class="breadcrumb">
            <li><a href="#"><i class="fa fa-dashboard"></i> Home</a></li>
            <li class="active">Dashboard</li>
          </ol>
        </section>
        -->
        <!-- Main content -->
        <section class="content">
          <!-- Small boxes (Stat box) -->
          <!-- /.row -->
          <!-- Main row -->
          <div class="row">
            <!-- Left col -->
            <div class="col-xs-12">
              <div class="box">
                <div class="box-header">
                  <h3 class="box-title">Relay Channels</h3>
                </div>
                <!-- /.box-header -->
                <div class="box-body" >
                  <table id="channelstable" class="table table-bordered table-hover">
                    <thead>
                      <tr>
                        <th>serverid</th>
                        <th>userid</th>
                        <th>deviceid</th>
                        <th>userport</th>
                        <th>deviceport</th>
                        <th>userfd</th>
                        <th>devicefd</th>
                        <th>useraddress</th>
                        <th>deviceaddress</th>
                        <th>createdate</th>
                      </tr>
                    </thead>
                    <tbody>
                      <c:forEach items="${channels}" var="ch">
                      <tr>
                        <td><c:out value="${ch.serverid}" /> </td>
                        <td><a href="/userinfo/${ch.userid}"><c:out value="${ch.userid}"/></a></td>
                        <td><a href="/devinfo/${ch.deviceid}"><c:out value="${ch.deviceid}"/></a></td>
                        <td><c:out value="${ch.userport}" /></td>
                        <td><c:out value="${ch.devport}" /> </td>
                        <td><c:out value="${ch.userfd}" /> </td>
                        <td><c:out value="${ch.devfd}" /> </td>
                        <td><c:out value="${ch.useraddress}" /> </td>
                        <td><c:out value="${ch.devaddress}" /> </td>
                        <td><c:out value="${ch.createDate}" /> </td>
                        </tr>
                        </c:forEach>
                      </tbody>
                    </table>
                  </div>
                </div>
              </div>
            </div>
            <!-- /.row (main row) -->
          </section>
          <!-- /.content -->
        </div>
        <!-- /.content-wrapper -->
        <%@ include file="foot.jspf" %>

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
      <script src="/resources/plugins/datatables/jquery.dataTables.js"></script>
      <script type="text/javascript">
          $("#channelmenu").addClass('active');
          $("#channelstable").DataTable();
      </script>
    </body>
  </html>