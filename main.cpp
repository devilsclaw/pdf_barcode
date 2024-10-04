#include <PDFWriter.h>
#include <PDFModifiedPage.h>
#include <PDFFormXObject.h>
#include <PDFPage.h>
#include <PageContentContext.h>
#include <PDFStream.h>
#include <DictionaryContext.h>
#include <PDFImageXObject.h>
#include <ProcsetResourcesConstants.h>
#include <XObjectContentContext.h>
#include <zint.h>

PDFFormXObject* createImageFormXObjectFromImageXObject(PDFWriter* inWriter, PDFImageXObject* imageXObject, ObjectIDType inFormXObjectID, uint32_t w, uint32_t h) {
	PDFHummus::DocumentContext& docCxt = inWriter->GetDocumentContext();
	PDFFormXObject* formXObject = NULL;
	do
	{

		formXObject = docCxt.StartFormXObject(PDFRectangle(0, 0, w, h), inFormXObjectID);
		XObjectContentContext* xobjectContentContext = formXObject->GetContentContext();

		xobjectContentContext->q();
		xobjectContentContext->cm(w, 0, 0, h, 0, 0);
		xobjectContentContext->Do(formXObject->GetResourcesDictionary().AddImageXObjectMapping(imageXObject));
		xobjectContentContext->Q();

		EStatusCode status = docCxt.EndFormXObjectNoRelease(formXObject);
		if (status != PDFHummus::eSuccess)
		{
			delete formXObject;
			formXObject = NULL;
			break;
		}
	} while (false);
	return formXObject;
}

static PDFImageXObject* createImageXObjectForBitmap(PDFWriter* inWriter, uint32_t w, uint32_t h, uint8_t* raster) {
	PDFImageXObject* imageXObject = NULL;
	PDFStream* imageStream = NULL;
	ObjectsContext& objCxt = inWriter->GetObjectsContext();
	EStatusCode status = eSuccess;

  // allocate image elements
  ObjectIDType imageXObjectObjectId = objCxt.GetInDirectObjectsRegistry().AllocateNewObjectID();

  objCxt.StartNewIndirectObject(imageXObjectObjectId);
  DictionaryContext* imageContext = objCxt.StartDictionary();

  // type
  imageContext->WriteKey("Type");
  imageContext->WriteNameValue("XObject");

  // subtype
  imageContext->WriteKey("Subtype");
  imageContext->WriteNameValue("Image");

  // Width
  imageContext->WriteKey("Width");
  imageContext->WriteIntegerValue(w);

  // Height
  imageContext->WriteKey("Height");
  imageContext->WriteIntegerValue(h);

  // Bits Per Component
  imageContext->WriteKey("BitsPerComponent");
  imageContext->WriteIntegerValue(8);

  // Color Space
  imageContext->WriteKey("ColorSpace");
  imageContext->WriteNameValue("DeviceRGB");

  // now for the image
  imageStream = objCxt.StartPDFStream(imageContext);
  IByteWriter* writerStream = imageStream->GetWriteStream();

  for(uint32_t pixel = 0; pixel < ((w * h) * 3);) {
    uint8_t r = raster[pixel++];
    uint8_t g = raster[pixel++];
    uint8_t b = raster[pixel++];
    writerStream->Write((IOBasicTypes::Byte*)&r, 1);
    writerStream->Write((IOBasicTypes::Byte*)&g, 1);
    writerStream->Write((IOBasicTypes::Byte*)&b, 1);
  }

  objCxt.EndPDFStream(imageStream);

  imageXObject = new PDFImageXObject(imageXObjectObjectId, KProcsetImageC);

	if (eFailure == status) {
		delete imageXObject;
		imageXObject = NULL;
	}
	delete imageStream;
	return imageXObject;
}

PDFFormXObject* CreateFormXObjectFromBitmap(PDFWriter* inWriter, uint32_t w, uint32_t h, uint8_t* raster, ObjectIDType inFormXObjectId) {
	PDFFormXObject* imageFormXObject = NULL;
	PDFImageXObject* imageXObject = NULL;

  do {
    // create image xobject based on image data
    PDFImageXObject* imageXObject = createImageXObjectForBitmap(inWriter, w, h, raster);
    if(!imageXObject)
      break;

    // create form
    imageFormXObject = createImageFormXObjectFromImageXObject(inWriter, imageXObject, inFormXObjectId == 0 ? inWriter->GetObjectsContext().GetInDirectObjectsRegistry().AllocateNewObjectID() : 0 , w, h);
  } while(false);

  delete imageXObject;
  return imageFormXObject;
}

EStatusCode placeBitmap(PDFWriter& inWriter, int page_num, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t* raster, bool original) {
	EStatusCode status;
	InputFile tiffFile;

	do {
    if(original) {
      PDFPage* page = new PDFPage();
      page->SetMediaBox(PDFRectangle(0,0,595,842));
      PageContentContext* pageContentContext = inWriter.StartPageContentContext(page);

      PDFFormXObject* imageFormXObject = CreateFormXObjectFromBitmap(&inWriter, w, h, raster, 0);
      if(!imageFormXObject) {
        printf("failed to create xobject from bitmap");
        break;
      }

      // place the image in page bottom left and discard form. scaled to quarter
      pageContentContext->q();
      pageContentContext->cm(1,0,0,1,x,y);
      pageContentContext->Do(page->GetResourcesDictionary().AddFormXObjectMapping(imageFormXObject->GetObjectID()));
      pageContentContext->Q();
      delete imageFormXObject;

      inWriter.EndPageContentContext(pageContentContext);
      inWriter.WritePageAndRelease(page);
    } else {
      PDFModifiedPage* page = new PDFModifiedPage(&inWriter, page_num);
      AbstractContentContext* pageContentContext = page->StartContentContext();

      PDFFormXObject* imageFormXObject = CreateFormXObjectFromBitmap(&inWriter, w, h, raster, 0);
      if(!imageFormXObject) {
        printf("failed to create xobject from bitmap");
        break;
      }

      // place the image in page bottom left and discard form. scaled to quarter
      pageContentContext->q();
      pageContentContext->cm(1,0,0,1,x,y);
      pageContentContext->Do(pageContentContext->GetResourcesDictionary()->AddFormXObjectMapping(imageFormXObject->GetObjectID()));
      pageContentContext->Q();
      delete imageFormXObject;

      page->EndContentContext();
      page->WritePage();
    }
	} while(false);

	return status;
}

int main(int argc, char** argv) {
  EStatusCode status = eSuccess;
  PDFWriter writer;
  bool original = (bool)atoi(argv[3]);

  do {
    status = writer.ModifyPDF(
      argv[1],
      ePDFVersion13,
      argv[2],
      LogConfiguration(true, true, argv[2])
    );

    if(status != eSuccess) {
      printf("failed to start PDF\n");
      break;
    }

    struct zint_symbol *symbol = NULL;
    symbol = ZBarcode_Create();
    if(symbol == NULL) {
      printf("failed to generate barcode\n");
      break;
    }

    int ret;
    symbol->symbology = BARCODE_EXCODE39;
    symbol->option_2 = -1;
    symbol->width = 151;
    symbol->height = 15;
    symbol->show_hrt = false;
    char barcode[] = "12345678";
    ret = ZBarcode_Encode_and_Buffer(symbol, (unsigned char *) barcode, sizeof(barcode), 0);
    placeBitmap(writer, 0, 0, 0, symbol->bitmap_width, symbol->bitmap_height, symbol->bitmap, original);

    ZBarcode_Delete(symbol);

    status = writer.EndPDF();
    if(status != eSuccess) {
      break;
    }

  } while(0);

  return 0;
}
