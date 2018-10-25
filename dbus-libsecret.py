import dbus

from dbus.mainloop.glib import DBusGMainLoop
DBusGMainLoop(set_as_default=True)

bus_name = 'org.freedesktop.secrets'
properties_name = 'org.freedesktop.DBus.Properties'
secret_collection_name = 'org.freedesktop.Secret.Collection'
secret_service_name = 'org.freedesktop.Secret.Service'
secret_item_name = 'org.freedesktop.Secret.Item'
secret_item_label_name = 'org.freedesktop.Secret.Item.Label'
prompt_name = 'org.freedesktop.Secret.Prompt'
default_collection_path = '/org/freedesktop/secrets/aliases/default'
secrets_path = '/org/freedesktop/secrets'

bus = dbus.SessionBus()
default_obj = bus.get_object(bus_name, default_collection_path)
service_obj = bus.get_object(bus_name, secrets_path)

properties = dbus.Interface(default_obj, properties_name)
collection = dbus.Interface(default_obj, secret_collection_name)
service = dbus.Interface(service_obj, secret_service_name)

session = service.OpenSession('plain', '')[1]

# all_items = service.SearchItems({})
# first = all_items[0][0]
all_properties = properties.GetAll(secret_collection_name)
all_items = all_properties['Items']
if all_properties['Locked']:
    print("Collection is locked. TODO: Try unlocking ...")
    prompt = service.Unlock(all_items)[1]
    prompt_obj = bus.get_object(bus_name, prompt)
    prompt_interface = dbus.Interface(prompt_obj, prompt_name)
    prompt_signal = None
    def prompt_done(dismissed, result):
        prompt_signal.remove()
        if dismissed:
            print("Unlock got cancelled")
        else:
            print("Unlock done with "+str(result))
    prompt_signal = prompt_interface.connect_to_signal("Completed", prompt_done)
    prompt_interface.Prompt("0")


for item_path in all_items:
    # print(item_path)
    item_obj = bus.get_object(bus_name, item_path)
    secret_properties = dbus.Interface(item_obj, properties_name)
    secret = secret_properties.GetAll(secret_item_name)
    print(secret['Label'])
    if secret['Locked']:
        print("Locked!")
        continue
    item_interface = dbus.Interface(item_obj, secret_item_name)
    secret_obj = item_interface.GetSecret(session)
    the_secret = "".join(map(chr, secret_obj[2]))
    print("secret: "+the_secret)

# item = bus.get_object(bus_name, first)
# item_properties = dbus.Interface(item, secret_item_name)
# item_secret = item.GetSecret(session)
# print(item_secret)
# print(item_secret)
# print(all_properties['Label'])

# print(properties.GetAll(secret_collection_name))
# print(properties.Get(secret_collection_name, 'Label'))
# print(all_items)


