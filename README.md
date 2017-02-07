# pebble-secret
Secret passwords holder application.

This is a simple app that will keep your secrets protected by a password.

The protection is done by encrypting the data with blowfish before saving to
storage.

When the application start it will ask for the encrypt/decrypt key.

![Welcome screen](https://raw.githubusercontent.com/BigET/pebble-secret/master/screenshots/welcome.png)

The encryption key input is done bit by bit in the welcome screen.

    * Up means a 1 bit.

    * Down means a 0 bit.

After accepting the key you get the list of secrets. Each secret has a title and a body.

![List of secrets](https://raw.githubusercontent.com/BigET/pebble-secret/master/screenshots/secret_list.png)

You can choose a secret to see the body of it.

![The secret body](https://raw.githubusercontent.com/BigET/pebble-secret/master/screenshots/delete_secret.png)

Here you can go back or delete the secret by holding the select button.

Once you will hit "back" from the list, the list will be encryptet back and saved to storage.

You can add more secrets with the configuration page.
