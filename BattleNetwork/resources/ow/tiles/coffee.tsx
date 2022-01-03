<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.4" tiledversion="1.4.1" name="coffee" tilewidth="33" tileheight="54" tilecount="4" columns="4" objectalignment="bottom">
 <tileoffset x="0" y="8"/>
 <grid orientation="orthogonal" width="64" height="32"/>
 <properties>
  <property name="Solid" type="bool" value="true"/>
 </properties>
 <image source="coffee.png" width="132" height="54"/>
 <tile id="0">
  <objectgroup draworder="index" id="2">
   <object id="2" x="-5" y="46">
    <polygon points="0,0 21,11 42,0 21,-11"/>
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
