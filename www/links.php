<?
require("header.php");
require("menu.php");
?>
<h1>Domain Name Relay Daemon</h1>

<? menu("Links"); ?>

<div id="content">
<h2>Links</h2>
<h3><a name="General DNS/DNRD links">General DNS/DNRD links</h3>

<ul>

<li><a href="http://dnrd.nevalabs.org">Brad Garcia's original DNRD
page</a></li>

<li><a href="http://www.dns.net/dnsrd/rfc">DNS related rfc's</a></li>

<li><a href="http://freshmeat.net/projects/dnrd">Freshmeat
page</a></li>

</ul>


<h3><a name="Authoritative DNS servers">Authoritative DNS servers</a></h3>
<ul>

<li><a
href="http://www.isc.org/products/BIND/bind9.html">bind</a></li>

<li><a href="http://cr.yp.to/djbdns.html">djbdns</a></li>

<li><a href="http://ldapdns.sourceforge.net">dapdns</a></li>

<li><a href="http://www.maradns.org/">aradns</a></li>

<li><a href="http://mydns.bboy.net/">mydns</a></li>

<li><a href="http://www.powerdns.com/">werdns</a></li>

<li><a href="http://www.posadis.org/">osadis</a></li>

</ul>


<h3><a name="Other DNS proxy servers">Other DNS proxy servers</a></h3>

<ul>

<li>
If you don't want any caching: <a
href="http://www.wolfermann.org/dnsproxy.html">dnsproxy</a> </li>

<li>
If you dont need domain "routing": <a
href="http://dproxy.sourceforge.net">dproxy</a> </li>

<li>
If you want integrated DHCP support: <a
href="http://www.thekelleys.org.uk/dnsmasq">dnsmasq</a> </li>

<li>
If you need to store the DNS cache to disk: <a
href="http://www.phys.uu.nl/%7Erombouts/pdnsd.html">pdnsd</a> </li>

</ul>

<p>Otherwise, use DNRD :)</p>

</div>

<? require("footer.php"); ?>
