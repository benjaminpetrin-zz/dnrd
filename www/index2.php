<?
require("header.php");
require("menu.php");
?>

<body>
<h1>Domain Name Relay Daemon</h1>

<? menu("Home"); ?>


<div id="content">
<h2><a name="about">What DNRD is</a></h2>
<p>
DNRD is a proxy name server.  To clients on your home network, it looks
just like a name server.  In reality, it forwards every DNS query to the
"real" DNS server, and forwards responses back to the client.
</p>
<p>
So, why would you want to use it?  DNRD was originally designed for
home networks where you might want to dial into more than one ISP (ie,
your home ISP and a dialup connection to your office).  The problem
with multiple dialups is that you need to change /etc/resolv.conf for
each one.  With DNRD, this is no longer necessary.
</p>
<p>
Your dialup machine will run DNRD (with appropriate options for forwarding
messages to the desired DNS servers).  All other machines on the home network,
including the dialup machine itself, will use the dialup machine as its DNS
server.  Configuring DNRD is a simple matter of passing the correct
command-line parameters.
</p>
<p>
Brad originally wrote DNRD to work with
<a href="http://cpwright.villagenet.com/mserver">masqdialer</a>.
It works very well with masqdialer, but should also work with other
dial-up systems.
</p>

</div>

<? require("footer.php"); ?>
