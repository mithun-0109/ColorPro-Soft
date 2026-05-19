from rest_framework import viewsets, status
from rest_framework.decorators import api_view, permission_classes
from rest_framework.permissions import AllowAny
from rest_framework.response import Response
from core.models import Batch, Roll
from core.serializers.roll import RollSerializer, RollCreateSerializer, RollBulkCreateSerializer


class RollViewSet(viewsets.ModelViewSet):
    """CRUD endpoints for rolls."""
    queryset = Roll.objects.all()
    serializer_class = RollSerializer
    filterset_fields = ['batch', 'status', 'shade_group', 'is_held']

    def get_serializer_class(self):
        if self.action == 'create':
            return RollCreateSerializer
        return RollSerializer


@api_view(['POST'])
def bulk_create_rolls(request):
    """
    Create multiple rolls for a batch at once.

    POST /api/rolls/bulk/
    {
        "batch_id": "<uuid>",
        "roll_numbers": ["R-001", "R-002", "R-003"]
    }
    """
    serializer = RollBulkCreateSerializer(data=request.data)
    serializer.is_valid(raise_exception=True)

    batch_id = serializer.validated_data['batch_id']
    roll_numbers = serializer.validated_data['roll_numbers']

    try:
        batch = Batch.objects.get(id=batch_id)
    except Batch.DoesNotExist:
        return Response(
            {'error': f'Batch {batch_id} not found'},
            status=status.HTTP_404_NOT_FOUND
        )

    created_rolls = []
    for idx, roll_num in enumerate(roll_numbers):
        roll, created = Roll.objects.get_or_create(
            batch=batch,
            roll_number=roll_num,
            defaults={'order': idx}
        )
        created_rolls.append(roll)

    return Response(
        RollSerializer(created_rolls, many=True).data,
        status=status.HTTP_201_CREATED
    )
