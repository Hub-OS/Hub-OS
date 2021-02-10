<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.4" tiledversion="1.4.1" name="gate" tilewidth="34" tileheight="52" tilecount="5" columns="5" objectalignment="topleft">
 <tileoffset x="14" y="-10"/>
 <properties>
  <property name="Solid" type="bool" value="true"/>
 </properties>
 <image source="gate.png" width="170" height="52"/>
 <tile id="0">
  <objectgroup draworder="index" id="2">
   <object id="2" x="-3" y="35">
    <polygon points="0,0 1,3 32,19 35,18 37,14 36,11 5,-5 2,-4"/>
   </object>
  </objectgroup>
  <animation>
   <frame tileid="0" duration="50"/>
   <frame tileid="1" duration="50"/>
   <frame tileid="2" duration="50"/>
   <frame tileid="3" duration="50"/>
  </animation>
 </tile>
</tileset>
