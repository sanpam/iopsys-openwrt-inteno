#!javascript
global.JUCI = require("../../../../tests/lib-juci"); 
require("../wifi"); 

describe("WIFI", function(){
	it("should have wireless config", function(done){
		$uci.sync("wireless").done(function(){
			expect($uci.wireless).to.be.an(Object); 
			done(); 
		}); 
	}); 
	it("should have at least one wireless device and interface defined", function(done){
		expect($uci.wireless["@wifi-device"]).to.be.an(Array); 
		expect($uci.wireless["@wifi-iface"]).to.be.an(Array); 
		expect($uci.wireless["@wifi-device"].length).not.to.be(0); 
		expect($uci.wireless["@wifi-iface"].length).not.to.be(0); 
		done(); 
	}); 
	it("should have easybox config present", function(done){
		$uci.sync("easybox").done(function(){
			expect($uci.easybox).to.be.an(Object); 
			done(); 
		}); 
	}); 
	it("should have easybox.settings section present", function(done){
		expect($uci.easybox["@settings"]).to.be.an(Array); 
		done(); 
	}); 
	it("should have hosts config present", function(done){
		$uci.sync("hosts").done(function(){
			expect($uci.hosts).to.be.an(Object); 
			done(); 
		}); 
	}); 
	it("should have wps.pbc rpc call", function(){
		expect($rpc.wps.pbc).to.be.a(Function); 
	}); 
}); 