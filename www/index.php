<?
require("header.php");
require("menu.php");
?>

<h1>Domain Name Relay Daemon</h1>

<? menu("Home"); ?>

<div id="content">

<h2><a name="about">What DNRD is</a></h2>

<p>
Domain Name Relay Daemon is a caching, forwarding DNS proxy
server. Most useful on vpn or dialup firewalls but it is also a nice DNS cache
for minor networks and workstations.
</p>

<h3>Features</h3>

<ul>

<li>Caching of DNS requests.</li>

<li>Support for backup DNS servers.</li>

<li>Uses random source port and random query ID's to prevent 
<a href="http://www.lurhq.com/cachepoisoning.html">cache poisoning</a>.
</li>

<li>Support for simple routing - specify different forward DNS servers
  for different domains.</li>

<li>Force authorative or unauthorative answers for specified
domains.</li>

<li>Share the /etc/hosts over the network.</li>

<li>Support for openbsd, freebsd and linux.</li>

<li>TCP support</li>

<li>DNS blacklist support</li>

</ul>

</div>

<? require("footer.php"); ?>
