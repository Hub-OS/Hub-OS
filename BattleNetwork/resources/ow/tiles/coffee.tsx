<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.2" tiledversion="1.3.2" name="coffee" tilewidth="33" tileheight="54" tilecount="4" columns="4">
 <tileoffset x="15" y="-6"/>
 <properties>
  <property name="Solid" type="bool" value="true"/>
 </properties>
 <image source="coffee.png" width="132" height="54"/>
 <tile id="0">
  <objectgroup draworder="index" id="2">
   <object id="2" x="0" y="46.0">
    <polygon points="0.0,0.0 16.0,8.0 32.0,0.0 16.0,-8.0"/>
   </object>
  </objectgroup>
  <animation>
   <frame tileid="0" duration="100"/>
   <frame tileid="1" duration="100"/>
   <frame tileid="2" duration="100"/>
   <frame tileid="3" duration="100"/>
  </animation>
 </tile>
</tileset>
