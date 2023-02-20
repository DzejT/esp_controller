include $(TOPDIR)/rules.mk

PKG_NAME:=esp_controller
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/esp_controller
	CATEGORY:=Base system
	TITLE:=esp_controller
	DEPENDS:=+libubox +libubus
endef

define Package/esp_controller/description
	Serial communication program
endef

define Package/esp_controller/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/esp_controller $(1)/usr/bin
	$(INSTALL_BIN) ./files/esp_controller.init $(1)/etc/init.d/esp_controller
	$(INSTALL_CONF) ./files/esp_controller.config $(1)/etc/config/esp_controller
endef

$(eval $(call BuildPackage,esp_controller))
