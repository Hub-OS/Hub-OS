<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.2" tiledversion="1.3.2" name="tree" tilewidth="44" tileheight="61" tilecount="4" columns="4">
 <tileoffset x="10" y="-6"/>
 <properties>
  <property name="Solid" type="bool" value="true"/>
 </properties>
 <image source="tree.png" width="176" height="61"/>
 <tile id="0">
  <objectgroup draworder="index" id="2">
   <object id="1" x="0.36607" y="41.0" width="41.0" height="19.1452">
    <ellipse/>
   </object>
  </objectgroup>
  <animation>
   <frame tileid="0" duration="200"/>
   <frame tileid="1" duration="200"/>
   <frame tileid="2" duration="200"/>
   <frame tileid="3" duration="200"/>
   <frame tileid="2" duration="200"/>
   <frame tileid="1" duration="200"/>
  </animation>
 </tile>
</tileset>
