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
            <div class="col-xs-12">
              <div class="box">
                <div class="box-header">
                  <h3 class="box-title">Log</h3>
                </div>
                <div class="box-body" >
                  <div class="form-inline">
                    <div class="input-group">
                      <label for="howmanylines" class="input-group-addon">Lines</label>
                      <input id="howmanylines" type="text" class="form-control" placeholder="<=500">
                    </div>
                    <div class="input-group">
                      <select class="form-control" id="serverselect">
                        <c:forEach items="${servers}" var="s">
                            <option value="${s.id}"><c:out value="${s.ip}"/></option>
                        </c:forEach>
                      </select>
                    </div>
                    <div class="input-group">
                      <div class="checkbox">
                        <label>
                          <input type="checkbox" id="isautoupdte" name="au"> Auto Update
                        </label>
                      </div>
                    </div>
                  </div>
                  <div id="logcontainer">
                  </div>
                </div>
                </div>
              </div>
            </div>
          </div>
        </section>
      </div>
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
    <!-- AdminLTE App -->
    <script src="/resources/dist/js/app.min.js"></script>
    <script  src="/resources/app/escape.js"></script>
    <script src="/resources/app/readlog.js"></script>
  </body>
</html>