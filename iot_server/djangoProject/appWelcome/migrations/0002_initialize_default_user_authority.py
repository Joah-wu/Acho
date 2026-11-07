from django.db import migrations


def initialize_default_user_authority(apps, schema_editor):
    UserAuthority = apps.get_model('appWelcome', 'UserAuthority')
    UserAuthority.objects.using(schema_editor.connection.alias).get_or_create(
        pk=1,
        defaults={'authority_level': 'user'},
    )


class Migration(migrations.Migration):
    dependencies = [
        ('appWelcome', '0001_initial'),
    ]

    operations = [
        # Keep the authority on rollback: deleting it would cascade to its users.
        migrations.RunPython(initialize_default_user_authority, migrations.RunPython.noop),
    ]
