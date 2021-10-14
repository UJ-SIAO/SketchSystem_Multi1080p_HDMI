-- Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2017.4 (win64) Build 2086221 Fri Dec 15 20:55:39 MST 2017
-- Date        : Sat Sep 25 19:01:39 2021
-- Host        : DESKTOP-AC8B3K3 running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode synth_stub
--               d:/Project/SketchSystem_Multi1080p_HDMI/SketchSystem.srcs/sources_1/bd/system/ip/system_c_rsample_0_0/system_c_rsample_0_0_stub.vhdl
-- Design      : system_c_rsample_0_0
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z020clg484-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity system_c_rsample_0_0 is
  Port ( 
    aclk : in STD_LOGIC;
    aresetn : in STD_LOGIC;
    s_axis_video_tdata : in STD_LOGIC_VECTOR ( 23 downto 0 );
    s_axis_video_tlast : in STD_LOGIC;
    s_axis_video_tready : out STD_LOGIC;
    s_axis_video_tuser : in STD_LOGIC;
    s_axis_video_tvalid : in STD_LOGIC;
    m_axis_video_tdata : out STD_LOGIC_VECTOR ( 15 downto 0 );
    m_axis_video_tlast : out STD_LOGIC;
    m_axis_video_tready : in STD_LOGIC;
    m_axis_video_tuser : out STD_LOGIC;
    m_axis_video_tvalid : out STD_LOGIC
  );

end system_c_rsample_0_0;

architecture stub of system_c_rsample_0_0 is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "aclk,aresetn,s_axis_video_tdata[23:0],s_axis_video_tlast,s_axis_video_tready,s_axis_video_tuser,s_axis_video_tvalid,m_axis_video_tdata[15:0],m_axis_video_tlast,m_axis_video_tready,m_axis_video_tuser,m_axis_video_tvalid";
attribute X_CORE_INFO : string;
attribute X_CORE_INFO of stub : architecture is "c_rsample,Vivado 2017.4";
begin
end;
