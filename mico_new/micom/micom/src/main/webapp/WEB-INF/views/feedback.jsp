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
        <!-- Main content -->
        <section class="content">
          <!-- Small boxes (Stat box) -->
          <!-- /.row -->
          <!-- Main row -->
          <div class="row">
            <!-- Left col -->
            <div class="col-xs-12">
              <div class="nav-tabs-custom">
                <!-- Tabs within a box -->
                <ul class="nav nav-tabs pull-left">
                  <li class="active"><a href="#feedback-tab" data-toggle="tab">Feedback</a></li>
                </ul>
                <div class="tab-content no-padding">
                  <!-- Morris chart Sales -->
                  <div class="chart tab-pane active" id="feedback-tab" style="position: relative;">
                    <table id="userinfo" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>User ID</th>
                          <th>Date</th>
                          <th>Content</th>
                        </tr>
                      </thead>
                      <tbody>
                        <c:forEach items="${feed}" var="f">
                        <tr>
                          <td><c:out value="${f.id}"/></td>
                          <td><a href="/userinfo/${f.userid}"><c:out value="${f.userid}"/></a></td>
                          <td><c:out value="${f.date}" /></td>
                          <td><c:out value="${f.content}" /></td>
                        </tr>
                        </c:forEach>
                      </tbody>
                    </table>
                    <div class="btn-group">
                      <c:if test="${page > 0}">
                      <button class="btn btn-default"><a href="/feedback/${page-1}">pre</a></button>
                      </c:if>
                      <c:if test="${!(page > 0)}">
                      <button class="btn btn-default">pre</button>
                      </c:if>
                      <button class="btn btn-default"><c:out value="${page}"/></button>
                      <c:if test="${page < pages}">
                      <button class="btn btn-default"><a href="/feedback/${page+1}">next</a></button>
                      </c:if>
                      <c:if test="${!(page < pages)}">
                      <button class="btn btn-default">next</button>
                      </c:if>
                      <button class="btn btn-default">All pages:<c:out value="${pages}"/></button>
                    </div>
                  </div>
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
    <script src="/dist/js/app.min.js"></script>
    <script type="text/javascript">
      $("#feedbackmenu").addClass('active');
    </script>
  </body>
</html>