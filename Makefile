include $(TOPDIR)/rules.mk

PKG_NAME:=mqttsub
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/mqttsub
	CATEGORY:=Base system
	TITLE:=mqttsub
	DEPENDS:=+libmosquitto-ssl +libuci +libcurl +libjson-c +libsqlite3
endef

define Package/mqttsub/description
	MQTT subscriber
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Package/mqttsub/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqttsub $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/mqttsub.init $(1)/etc/init.d/mqttsub
	$(INSTALL_CONF) ./files/mqttsub.config $(1)/etc/config/mqttsub
endef

$(eval $(call BuildPackage,mqttsub))
