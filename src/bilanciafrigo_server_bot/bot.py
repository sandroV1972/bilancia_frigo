import os
import json
from telegram import Update
from telegram.ext import ApplicationBuilder, CommandHandler, ContextTypes
from mqtt_handler import publish_message
from utils import load_users, save_user

TOKEN = os.getenv("TELEGRAM_TOKEN")
users = load_users()

async def start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    if context.args:
        device_id = context.args[0]
        chat_id = update.effective_chat.id
        save_user(device_id, chat_id)
        await update.message.reply_text(f"🔗 Registrazione completata per: {device_id}")
        publish_message(device_id, "registered")
    else:
        await update.message.reply_text("⚠️ Devi fornire un ID dispositivo. Es: /start BILANCIA-1234")

if __name__ == '__main__':
    app = ApplicationBuilder().token(TOKEN).build()
    app.add_handler(CommandHandler("start", start))
    print("🤖 Bot avviato...")
    app.run_polling()
