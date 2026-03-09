"""
Comando de diagnóstico: verifica si un usuario tiene estación y datos en la BD.
Uso: python3 manage.py check_user_data ma.gomeza1
"""
from django.core.management.base import BaseCommand
from django.contrib.auth.models import User
from receiver.models import Station, Data


class Command(BaseCommand):
    help = "Verifica estaciones y datos de un usuario en la BD"

    def add_arguments(self, parser):
        parser.add_argument("username", type=str, help="Nombre de usuario (ej. ma.gomeza1)")

    def handle(self, *args, **options):
        username = options["username"]
        self.stdout.write(f"Usuario: {username}\n")

        user = User.objects.filter(username=username).first()
        if not user:
            self.stdout.write(self.style.ERROR(f"  El usuario '{username}' NO existe en Django (auth_user)."))
            self.stdout.write("  Créalo desde la web (Usuarios → Registrar) o con createsuperuser/shell.")
            return

        self.stdout.write(self.style.SUCCESS(f"  Usuario existe (id={user.id})"))

        stations = Station.objects.filter(user=user)
        if not stations:
            self.stdout.write(
                self.style.WARNING(
                    "  No hay ninguna Station para este usuario. "
                    "La estación se crea cuando el receiver procesa el primer mensaje MQTT con este usuario en el tópico."
                )
            )
            self.stdout.write(
                "  Comprueba: 1) Arduino publica en .../ma.gomeza1/out  2) Receiver (start_mqtt) está corriendo  3) nohup.out del receiver sin errores."
            )
            return

        self.stdout.write(self.style.SUCCESS(f"  Estaciones: {stations.count()}"))
        for s in stations:
            loc = s.location
            self.stdout.write(f"    - {loc.country.name}/{loc.state.name}/{loc.city.name}")

        total = Data.objects.filter(station__user=user).count()
        self.stdout.write(self.style.SUCCESS(f"  Total registros Data: {total}"))
        if total == 0:
            self.stdout.write(self.style.WARNING("  Aún no hay datos. Espera a que el Arduino envíe y el receiver los guarde."))
