from rest_framework.decorators import api_view, permission_classes
from rest_framework.permissions import AllowAny
from rest_framework.response import Response
from rest_framework import status
from django.utils import timezone

from core.models import Batch, Roll, DeviceStatus
from core.serializers.roll import RollSerializer


@api_view(['GET'])
@permission_classes([AllowAny])
def device_roll_queue(request, batch_id):
    """
    Get the roll queue for ESP32 device.
    Returns ordered list of rolls (pending + held) for scanning.

    GET /api/device/batch/<batch_id>/rolls/
    """
    # Track device activity implicitly when it asks for its queue
    DeviceStatus.ping()

    try:
        batch = Batch.objects.get(id=batch_id)
    except Batch.DoesNotExist:
        return Response(
            {'error': f'Batch {batch_id} not found'},
            status=status.HTTP_404_NOT_FOUND
        )

    # Return all rolls ordered by position, with status info
    rolls = batch.rolls.all().order_by('order', 'created_at')

    roll_data = []
    for roll in rolls:
        roll_data.append({
            'id': str(roll.id),
            'roll_number': roll.roll_number,
            'order': roll.order,
            'status': roll.status,
            'is_held': roll.is_held,
            'scan_count': roll.scan_count,
        })

    return Response({
        'batch_id': str(batch_id),
        'batch_name': batch.name,
        'total_rolls': len(roll_data),
        'rolls': roll_data,
    })


@api_view(['PATCH'])
@permission_classes([AllowAny])
def device_hold_roll(request, roll_id):
    """
    Mark a roll as held (ESP32 Hold Roll button).

    PATCH /api/device/roll/<roll_id>/hold/
    """
    try:
        roll = Roll.objects.get(id=roll_id)
        roll.is_held = True
        roll.save(update_fields=['is_held'])
        return Response({
            'status': 'ok',
            'roll_id': str(roll.id),
            'roll_number': roll.roll_number,
            'is_held': True,
        })
    except Roll.DoesNotExist:
        return Response(
            {'error': f'Roll {roll_id} not found'},
            status=status.HTTP_404_NOT_FOUND
        )


@api_view(['POST'])
@permission_classes([AllowAny])
def device_ping(request):
    """
    Heartbeat ping from ESP32 to track connection status.
    
    POST /api/device/ping/
    """
    obj = DeviceStatus.ping()
    return Response({'status': 'ok', 'last_seen': obj.last_ping})


@api_view(['GET'])
@permission_classes([AllowAny])
def get_device_status(request):
    """
    Check if the ESP32 is currently online.
    
    GET /api/device/status/
    """
    obj, _ = DeviceStatus.objects.get_or_create(id=1)
    return Response({
        'name': obj.name,
        'online': obj.is_online,
        'last_seen': obj.last_ping
    })
