<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.4" tiledversion="1.4.1" name="gate" tilewidth="34" tileheight="52" tilecount="5" columns="5" objectalignment="bottom">
 <tileoffset x="0" y="17"/>
 <properties>
  <property name="Solid" type="bool" value="true"/>
 </properties>
 <image source="gate.png" width="170" height="52"/>
 <tile id="0">
  <objectgroup draworder="index" id="3">
   <object id="2" x="-8.5" y="19.5">
    <polygon points="25.5,15.5 -6.5,31.5 25.5,47.5 57.5,31.5"/>
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
