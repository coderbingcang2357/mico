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
    <script type="text/javascript" src="./resources/vue/vue.js"></script>
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

    <style type="text/css">
        td {
            word-break: break-all;
        }
    </style>

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
          <div class="box">
              <div class="box-header">
                <h3 class="box-title">Push Messages</h3>
                 <button id="addnewpm" class="btn box-primary glyphicon glyphicon-plus" data-toggle="modal" data-target="#addnewpushmessage"></button>
              </div>
              <!-- /.box-header -->
              <div class="box-body" >
                <div id="pushmessages">
                  <table id="pushmessagetable" class="table table-bordered table-hover">
                      <thead>
                        <tr>
                          <th>ID</th>
                          <th>Message</th>
                          <th>Create Date</th>
                          <th></th>
                        </tr>
                      </thead>
                      <tbody>
                        <tr v-for="(msg, index) in messages">
                           <td>{{msg.id}}</a></td>
                           <td>{{msg.message}}</td>
                           <td>{{msg.createdate}}</td>
                           <td><i style="display:none" v-bind:id="'send-busing-'+msg.id" class="fa fa-refresh fa-spin"></i>
                              <button v-bind:id="'send-' + msg.id" v-if="msg.shouldsend === 0" class="btn btn-default" v-on:click="sendpushmessage(index)">Send</button>

                             <i style="display:none" v-bind:id="'delete-busing-'+msg.id" class="fa fa-refresh fa-spin"></i>
                             <button v-bind:id="'delete-' + msg.id" class="btn btn-default" v-on:click="deletemessage(index)">delete</button></td>
                        </tr>
                      </tbody>
                  </table>
                </div>
              </div>
          </div>
         </section>
          <!-- /.content -->
        </div>
        <!-- /.content-wrapper -->
        <%@ include file="foot.jspf" %>

      </div>
      <!-- 添加新推送消息的对话框 -->
      <div class="modal fade" data-backdrop="static"  data-keyboard="false" id="addnewpushmessage" tabindex="-1" role="dialog" aria-labelledby="exampleModalLabel">
            <div class="modal-dialog modal-lg" role="document">
              <div class="modal-content">
                <div class="modal-header">
                  <button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>
                  <h4 class="modal-title" id="exampleModalLabel">Create Push Message</h4>
                </div>
                <div class="box box-primary">
                  <div class="box-body" style="height: 300px; overflow: auto;">
                    <textarea style="width:100%;" id="inputpushmessage"></textarea>
                  </div>
                </div>
                <div class="modal-footer">
                  <button type="button" class="btn btn-default" data-dismiss="modal">Close</button>
                  <button type="button" class="btn btn-primary" id="addnewpushmessage_commit">Submit</button>
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
      <script src="/resources/app/stringformat.js"></script>
      <script src="/resources/app/infobox.js"></script>
      <script src="/resources/app/pushmessage.js"></script>
      <script src="/resources/plugins/datatables/jquery.dataTables.js"></script>
      <script type="text/javascript">
          $("#pushmessagemenu").addClass('active');
          $("#pushmessagetable").DataTable();
      </script>
    </body>
  </html>
