#!/bin/bash
# Script rà soát chất lượng mã nguồn và kiểm thử

echo "==========================================="
echo "1. ĐANG CHẠY RÀ SOÁT MÃ NGUỒN (LINTING)..."
echo "==========================================="
uv run ruff check .

if [ $? -eq 0 ]; then
    echo "✅ Linting PASSED: Code tuân thủ chuẩn."
else
    echo "❌ Linting FAILED: Vui lòng sửa các lỗi ruff ở trên."
    exit 1
fi

echo ""
echo "==========================================="
echo "2. ĐANG CHẠY KIỂM THỬ (TESTING)..."
echo "==========================================="
uv run pytest --cov=app tests/

if [ $? -eq 0 ]; then
    echo "✅ Testing PASSED: Tất cả kịch bản kiểm thử thành công."
else
    echo "❌ Testing FAILED: Có lỗi xảy ra trong kịch bản kiểm thử."
    exit 1
fi

echo ""
echo "🎉 HỆ THỐNG ĐÃ ĐẠT CHUẨN SQA!"
