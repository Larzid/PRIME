all:
	@echo For Mac OS X use \"$(MAKE) -f makefile.osx\"
	@echo For Linux use \"$(MAKE) -f makefile.std\"
	@echo Optionally append target argument
	@echo
	@echo Valid targets:
	@echo ' (nothing)   -  default'
	@echo ' all         -  default'
	@echo ' single      -  creates binary for single user'
	@echo ' multi       -  creatres distribution with shared high score'
	@echo '               table; suitable for placing on servers'
	@echo ' install     -  places files in system locations'
	@echo '               (needed for 'multi' target)'
	@echo ' app-osx     -  on Mac OS X only this builds an application'
	@echo ' console     -  creates binary for single user without support for'
	@echo '               Necklace of the Eye graphical frontend compiled in'
	@echo ' multiconsole-  for Linux only this creates binary suitable for use'
	@echo '                by many users but without graphical frontend support'
	@echo
