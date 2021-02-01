<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<%@ taglib prefix="sec" uri="http://www.springframework.org/security/tags" %>
<%@ page language="java" import="java.util.*" pageEncoding="UTF-8"%>
<%@ page session="true" %>
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
  </head>
  <body class="hold-transition skin-blue sidebar-mini fixed">
    <div class="wrapper">
      <%@ include file="header.jspf" %>
      <!-- Left side column. contains the logo and sidebar -->
      <%@ include file="sidebar.jspf" %>
      <div class="content-wrapper">
        <sec:authorize access="hasRole('ROLE_ADMIN')">
        <div style="font-family:Consolas, 微软雅黑;font-size:15px; overflow:scroll;">
          <H1>1. 腾迅云服务器</H1>
          <TABLE CELLPADDING="4" class="table table-bordered table-hover" style="width:2000px;">
            <TR>
              <TH>操作系统</TH>
              <TH>CPU</TH>
              <TH>内存</TH>
              <TH>带宽</TH>
              <TH>功能</TH>
              <TH>公网IP</TH>
              <TH>内网IP</TH>
              <TH>数据盘大小</TH>
              <TH>帐号</TH>
              <TH>密码</TH>
              <TH>mysql帐号密码</TH>
              <TH>redis端口</TH>
              <TH>redis密码</TH>
            </TR>
            <TR>
              <TD>ubuntu14.04x86_64</TD>
              <TD ALIGN="right">2</TD>
              <TD>4</TD>
              <TD>3</TD>
              <TD>测试服务器</TD>
              <TD COLSPAN="2">203.195.150.199</TD>
              <TD>100</TD>
              <TD>wangqiaosheng</TD>
              <TD>x88c99a</TD>
              <TD>root(SSDD5512)</TD>
            </TR>
            <TR>
              <TD>ubuntu14.04x86_64</TD>
              <TD ALIGN="right">2</TD>
              <TD>4</TD>
              <TD>3</TD>
              <TD>状态服务器, 保存在线用户,设备信息</TD>
              <TD>203.195.191.182</TD>
              <TD>10.221.220.167</TD>
              <TD>100</TD>
              <TD>wangqiaosheng</TD>
              <TD>x88c99a</TD>
              <TD>root(SSDD5512)</TD>
              <TD>13879</TD>
              <TD>3ETkyXt6BFFgVf8lJOaomWLPRiXs7Dxb</TD>
            </TR>
            <TR>
              <TD>ubuntu14.04x86_64</TD>
              <TD ALIGN="right">2</TD>
              <TD>4</TD>
              <TD>3</TD>
              <TD>场景文件服务器</TD>
              <TD>203.195.204.201</TD>
              <TD>10.221.53.144</TD>
              <TD>100</TD>
              <TD>wangqiaosheng</TD>
              <TD>x88c99a</TD>
            </TR>
            <TR>
              <TD>ubuntu14.04x86_64</TD>
              <TD ALIGN="right">2</TD>
              <TD>4</TD>
              <TD>3</TD>
              <TD>发布, 应用服务器</TD>
              <TD>203.195.237.53</TD>
              <TD>10.232.47.24</TD>
              <TD>100</TD>
              <TD>wangqiaosheng</TD>
              <TD>x88c99a</TD>
            </TR>
            <TR>
              <TD>ubuntu14.04x86_64</TD>
              <TD ALIGN="right">2</TD>
              <TD>4</TD>
              <TD>3</TD>
              <TD>数据库服务器</TD>
              <TD>203.195.236.218</TD>
              <TD>10.221.229.98</TD>
              <TD>100</TD>
              <TD>wangqiaosheng</TD>
              <TD>x88c99a</TD>
              <TD>root(SSDD5512), ctdb(SSDD5512)</TD>
            </TR>
          </TABLE>
          <H1>2. 公司内部虚拟机</H1>
          <TABLE CELLPADDING="4" class="table table-bordered table-hover">
            <TR>
              <TH>IP</TH>
              <TH>帐号</TH>
              <TH>密码</TH>
              <TH>mysql帐号密码</TH>
              <TH>redis端口</TH>
              <TH>redis密码</TH>
            </TR>
            <TR>
              <TD>10.1.240.39</TD>
              <TD>wangqiaosheng</TD>
              <TD>Qx8850*</TD>
              <TD>root(SSDD5512), ctdb(SSDD5512)</TD>
              <TD>6379</TD>
              <TD>3ETkyXt6BFFgVf8lJOaomWLPRiXs7Dxb</TD>
            </TR>
            <TR>
              <TD>10.1.240.42</TD>
              <TD>wangqiaosheng</TD>
              <TD>Qx8850*</TD>
            </TR>
            <TR>
              <TD>10.1.240.44</TD>
              <TD>wangqiaosheng</TD>
              <TD>Qx8850*</TD>
            </TR>
            <TR>
              <TD>10.1.240.47</TD>
              <TD>wangqiaosheng</TD>
              <TD>Qx8850*</TD>
            </TR>
          </TABLE>
        </div>
        </sec:authorize>
      </div>
      <%@ include file="foot.jspf" %>
    </div>
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
    <script type="text/javascript">
      $("#serverlistmenu").addClass('active');
    </script>
  </body>
</html>
