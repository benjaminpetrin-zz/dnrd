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
Domain Name Relay Daemon is a caching, forwarding DNS proxy
server. Most useful on vpn or dialup firewalls but is a nice DNS cache
for minor networks and workstations.
<h3>Features</h3>
<ul>
<li>Caching of DNS requests</li>
<li>Support for backup DNS servers</li>
<li>Support for simple routing - specify different forward DNS servers
  for different domains. This great for private networks, vpn's and
  offline sites.</li>
<li>Force authorative or unauthorative answers for specified
domains. Can be used as a very simple dns server for home
networks</li>
<li>share the /etc/hosts over the network.</li>
<li>support for openbsd, freebsd and linux</li>
</ul>

</div>

<? require("footer.php"); ?>
