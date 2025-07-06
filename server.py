import base64
import cv2
import numpy as np
from fastapi import FastAPI
from pydantic import BaseModel # 1. 导入BaseModel
from paddleocr import PaddleOCR

# 初始化FastAPI应用和PaddleOCR
app = FastAPI()
# 只需初始化一次，指定使用中文/英文模型
ocr = PaddleOCR(use_angle_cls=True, lang='ch') 

# 2. 定义一个模型，描述我们期望接收的JSON结构
class OCRRequest(BaseModel):
    image_base64: str

@app.post("/ocr")
async def process_ocr(request: OCRRequest):
    """接收图像，进行OCR，返回结果"""
    # 1. 读取上传的图像数据
    try:
        img_data = base64.b64decode(request.image_base64)
    except Exception as e:
        return {"error": "Invalid Base64 string"}

    nparr = np.frombuffer(img_data, np.uint8)
    img_cv = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

    if img_cv is None:
        return {"error": "Failed to decode image"}

    # 2. 使用PaddleOCR进行识别
    result = ocr.ocr(img_cv, cls=True)

    # 3. 将识别结果绘制到图像上并提取文本
    boxes = [line[0] for line in result[0]]
    texts = [line[1][0] for line in result[0]]
    scores = [line[1][1] for line in result[0]]
    
    img_with_boxes = img_cv.copy()
    for i in range(len(boxes)):
        # 绘制边界框
        #if scores[i] > 0.5:
            box = boxes[i]
            cv2.polylines(img_with_boxes, [np.array(box).astype(np.int32)], True, color=(0, 255, 0), thickness=2)

    cv2.imwrite('result.jpg',img_with_boxes)
    # 4. 将处理后的图像编码为Base64字符串
    _, buffer = cv2.imencode('.jpg', img_with_boxes)
    img_base64 = base64.b64encode(buffer).decode('utf-8')

    # 5. 构造并返回JSON结果
    return {
        "text": "\n".join(texts),
        "annotated_image": img_base64
    }

if __name__ == "__main__":
    import uvicorn
    # 运行服务，监听在本地8000端口
    uvicorn.run(app, host="127.0.0.1", port=8000)