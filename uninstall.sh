


sudo naga -kill
sleep 1
sudo rm -f /usr/local/bin/naga* /etc/udev/rules.d/80-naga.rules ~/.config/autostart/naga.desktop
rm -rf ~/.config/naga
sudo gpasswd -d $(whoami) razer

