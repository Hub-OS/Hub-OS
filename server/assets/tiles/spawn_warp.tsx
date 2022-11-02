<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.9" tiledversion="1.9.2" name="hp_warp" tilewidth="92" tileheight="51" tilecount="5" columns="5" objectalignment="top">
 <image source="spawn_warp.png" width="460" height="51"/>
 <tile id="1">
  <objectgroup draworder="index" id="2">
   <object id="1" x="26" y="9" width="40" height="25">
    <ellipse/>
   </object>
  </objectgroup>
  <animation>
   <frame tileid="1" duration="300"/>
   <frame tileid="2" duration="100"/>
   <frame tileid="3" duration="100"/>
   <frame tileid="4" duration="400"/>
   <frame tileid="3" duration="100"/>
   <frame tileid="2" duration="100"/>
  </animation>
 </tile>
</tileset>
