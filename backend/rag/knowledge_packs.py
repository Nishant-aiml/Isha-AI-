"""
Pre-bundled Domain Knowledge Packs for ISHA AI.
Matches specifications in isha ai.md Section 10 and core/retrieval/retrieval_engine.cpp:
- kisan_saathi: Comprehensive agricultural guidance, crop diseases, fertilizer, schemes.
- swasthya: Emergency medicine, first aid, pediatric dosages, seasonal epidemics.
- bharat_core: Indian Constitution, citizen rights, consumer law, FIRs, emergency numbers.
- shiksha: STEM fundamentals, physics laws, chemical reactions, biology formulas.
"""

from typing import List, Dict, Any

DOMAIN_PACKS: Dict[str, Dict[str, Any]] = {
    "kisan_saathi": {
        "title": "Kisan Saathi (Agriculture & Crop Care)",
        "description": "Offline agricultural advisor: crop diseases, soil health, fertilizer dosing, weather precautions, and government schemes.",
        "icon": "🌾",
        "chunks": [
            {
                "id": "kisan_001",
                "title": "Wheat Nitrogen Deficiency & Correction",
                "text": "Nitrogen deficiency in wheat causes yellowing of lower leaves, stunted plant height, and reduced tillering. Correction: Apply 60 kg/ha urea at basal dose and top-dress remaining 60 kg/ha in two splits during crown root initiation (CRI, 21 days) and heading stage. Ensure soil has adequate moisture."
            },
            {
                "id": "kisan_002",
                "title": "Wheat Rust (Yellow and Brown) Identification & Treatment",
                "text": "Yellow/Stripe Rust appears as bright yellow linear pustules arranged in parallel lines on leaf blades. Brown/Leaf rust appears as scattered circular orange-brown pustules. Spray Propiconazole 25% EC at 1 ml/litre or Tebuconazole 25.9% EC at 1 ml/litre immediately upon sighting. Repeat in 15 days if infection persists."
            },
            {
                "id": "kisan_003",
                "title": "Rice Blast Disease Management",
                "text": "Rice Blast (Magnaporthe oryzae) manifests as spindle-shaped or diamond lesions with greyish centres and brown margins on leaves, collars, and neck. Spray Tricyclazole 75% WP at 0.6 g/litre water or Isoprothiolane 40% EC at 1.5 ml/litre. Avoid excessive nitrogen fertilizer during cloudy weather."
            },
            {
                "id": "kisan_004",
                "title": "Phosphorus and Potassium Deficiencies",
                "text": "Phosphorus deficiency shows purplish or dark bronze discoloration on leaf edges and stunted roots; correct by applying Diammonium Phosphate (DAP) at 100 kg/ha at sowing. Potassium deficiency causes burning/scorching along leaf tips and margins; correct with MOP (Muriate of Potash) before flowering stage."
            },
            {
                "id": "kisan_005",
                "title": "Cotton Pink Bollworm Control",
                "text": "Pink bollworm damages flower buds ('rosette flowers') and seeds inside developing bolls. Install pheromone traps at 5 traps/acre for monitoring. If catches exceed 8 moths/trap for 3 consecutive days, spray Profenofos 50% EC at 2 ml/litre or Chlorantraniliprole 18.5% SC at 0.3 ml/litre."
            },
            {
                "id": "kisan_006",
                "title": "Drip Irrigation Water Savings & Soil Testing",
                "text": "Drip irrigation delivers water directly to plant root zones, saving 40-60% water over flood irrigation while boosting crop yields by 20-30%. Soil pH between 6.0 and 7.5 is optimal for nutrient uptake. Apply agricultural lime (CaCO3) for acidic soils (pH < 6.0) and gypsum for alkaline soils (pH > 8.2)."
            },
            {
                "id": "kisan_007",
                "title": "PM-Kisan & Pradhan Mantri Fasal Bima Yojana",
                "text": "Under PM-KISAN, eligible landholding farmer families receive direct income support of ₹6,000 per year in three equal instalments of ₹2,000 via DBT. Under PMFBY, premium rates are 2% for Kharif food/oilseeds, 1.5% for Rabi crops, and 5% for annual commercial/horticultural crops, covering post-harvest and localized calamities."
            },
            {
                "id": "kisan_008",
                "title": "Organic Pest Repellents (Neemastra & Bramhastra)",
                "text": "Neemastra formula: 5 kg crushed neem leaves + 5 litres cow urine + 2 kg cow dung fermented in 100 litres water for 48 hours; controls sucking pests and caterpillars. Bramhastra: Boil crushed neem, custard apple, papaya, and guava leaves in cow urine; spray at 30 ml per litre for borers."
            }
        ]
    },
    "swasthya": {
        "title": "Swasthya Rakshak (Health & First Aid)",
        "description": "Essential medical guidance: first aid, pediatric dosage guidelines, disease prevention, and emergency response.",
        "icon": "🩺",
        "chunks": [
            {
                "id": "swasthya_001",
                "title": "Pediatric Paracetamol Dosage Protocol",
                "text": "Paracetamol dosage for children is strictly weight-based: 10 to 15 mg per kg body weight per dose, administered every 4 to 6 hours as needed. Maximum 4 doses (or 60 mg/kg) within 24 hours. Always use an oral dosing syringe, never a household teaspoon. Never combine with other multi-symptom cold medicines containing paracetamol."
            },
            {
                "id": "swasthya_002",
                "title": "Diarrhea & Dehydration ORS Preparation",
                "text": "Oral Rehydration Salts (ORS) is the cornerstone treatment for acute diarrhea. Dissolve 1 full WHO-formula ORS sachet into exactly 1 litre of clean boiled and cooled drinking water. Stir until completely dissolved. Give frequent small sips. Accompany with Zinc supplement tablets (20 mg daily for children >6 months, 10 mg for <6 months) for 14 continuous days."
            },
            {
                "id": "swasthya_003",
                "title": "First Aid for Burns & Scalds",
                "text": "Immediately immerse the burned area under cold, gentle running water for at least 20 minutes to halt thermal tissue destruction. Remove tight jewellery before swelling occurs. Cover loosely with sterile cling film or clean non-adherent cloth. DO NOT apply ice, toothpastes, turmeric, butter, or oil. Seek urgent medical attention if the burn exceeds the size of the victim's palm."
            },
            {
                "id": "swasthya_004",
                "title": "Dengue Warning Signs & Monitoring",
                "text": "Critical phase of Dengue occurs between days 3 and 7 when fever drops. Danger signs requiring immediate hospitalization: severe persistent abdominal pain, recurrent vomiting, bleeding gums or nose, mucosal hemorrhage, extreme lethargy, sudden drop in platelet count accompanied by rise in hematocrit. Avoid Aspirin and Ibuprofen; only use Paracetamol."
            },
            {
                "id": "swasthya_005",
                "title": "Adult Hands-Only CPR Protocol",
                "text": "If an adult collapses and is unresponsive and not breathing normally: 1. Call emergency services (112 / 108) immediately. 2. Position the heel of your hand on the centre of the patient's chest. 3. Push hard and fast at a rate of 100 to 120 compressions per minute to a depth of 5-6 cm (about 2 inches). Allow complete chest recoil between each push."
            },
            {
                "id": "swasthya_006",
                "title": "Hypertension & Diabetes Dietary Management",
                "text": "Hypertension: Restrict dietary sodium to less than 2 grams per day (under 5 grams of table salt). Increase dietary potassium through bananas, green leafy vegetables, and lentils. Diabetes: Prioritize high-fiber complex carbohydrates (whole grains, millets), avoid refined sugars and sugary beverages, and engage in minimum 150 minutes of moderate aerobic exercise weekly."
            },
            {
                "id": "swasthya_007",
                "title": "Ayushman Bharat PM-JAY Scheme Details",
                "text": "Ayushman Bharat Pradhan Mantri Jan Arogya Yojana (PM-JAY) provides cashless secondary and tertiary hospitalization cover of up to ₹5,00,000 per family per year across empaneled public and private hospitals across India. Covers pre-existing conditions from day one, including pre- and post-hospitalization expenses for up to 15 days."
            }
        ]
    },
    "bharat_core": {
        "title": "Bharat Vidhi & Civic Core",
        "description": "Indian Constitution, legal safeguards, consumer protection, police FIR procedures, and citizen helplines.",
        "icon": "⚖️",
        "chunks": [
            {
                "id": "bharat_001",
                "title": "National Emergency Helpline Numbers",
                "text": "National Emergency Unified Helpline: 112 (Police, Fire, Ambulance). Police: 100, Fire: 101, Ambulance: 108. Women Helpline (National): 1091 and 181. National Cyber Crime Reporting Portal: 1930. Childline Helpline: 1098. Senior Citizen Helpline (Elderline): 14567. Railway Security Helpline: 139."
            },
            {
                "id": "bharat_002",
                "title": "Filing a Police FIR (First Information Report)",
                "text": "Section 154 CrPC / BNSS mandates that an officer in charge of a police station must register an FIR upon information of a cognizable offence. A citizen has the legal right to receive a signed copy of the FIR immediately and free of cost. If a police station refuses to register an FIR, the complainant can send the substance of information in writing via registered post to the Superintendent of Police (SP) or file a Zero FIR at any nearby station."
            },
            {
                "id": "bharat_003",
                "title": "Fundamental Rights Guaranteed by Constitution of India",
                "text": "Part III of the Indian Constitution enshrines 6 Fundamental Rights: Right to Equality (Articles 14-18), Right to Freedom (Articles 19-22, including speech and expression), Right against Exploitation (Articles 23-24), Right to Freedom of Religion (Articles 25-28), Cultural and Educational Rights (Articles 29-30), and Right to Constitutional Remedies (Article 32 - enabling citizens to approach Supreme Court directly via habeas corpus, mandamus, prohibition, quo warranto, and certiorari)."
            },
            {
                "id": "bharat_004",
                "title": "Right to Information (RTI Act 2005) Process",
                "text": "Any Indian citizen can request information from a Public Authority under RTI Act 2005. Standard application fee is ₹10 (exempt for Below Poverty Line applicants). The Public Information Officer (PIO) must supply the information within 30 days. If the matter involves life or liberty of a person, information must be provided within 48 hours."
            },
            {
                "id": "bharat_005",
                "title": "Consumer Protection Act 2019 Remedies",
                "text": "Under Consumer Protection Act 2019, consumers can file e-complaints via the e-Daakhil portal without needing a lawyer. District Commissions adjudicate claims up to ₹50 Lakh, State Commissions up to ₹2 Crore, and National Commission above ₹2 Crore. Covers unfair trade practices, misleading advertisements, defective goods, and deficiency in services."
            }
        ]
    },
    "shiksha": {
        "title": "Shiksha STEM & Reasoning",
        "description": "Scientific constants, physics laws, calculus, biological pathways, and competitive exam reasoning concepts.",
        "icon": "🎓",
        "chunks": [
            {
                "id": "shiksha_001",
                "title": "Newton's Laws of Motion & Momentum",
                "text": "Newton's First Law (Law of Inertia): An object remains at rest or in uniform motion along a straight line unless acted upon by an external net force. Second Law: Net force equals the rate of change of linear momentum F = dp/dt = m*a. Third Law: For every action force, there is an equal and opposite reaction force acting on different bodies simultaneously."
            },
            {
                "id": "shiksha_002",
                "title": "Thermodynamics Laws & Carnot Efficiency",
                "text": "Zeroth Law: Defines temperature via thermal equilibrium. First Law (Conservation of Energy): ΔU = Q - W. Second Law: Total entropy of an isolated system never decreases; heat cannot spontaneously flow from cooler to hotter body without work. Carnot engine maximum efficiency: η = 1 - (T_cold / T_hot), where temperatures are in Kelvin."
            },
            {
                "id": "shiksha_003",
                "title": "Photosynthesis Light and Dark Reactions",
                "text": "Photosynthesis equation: 6CO2 + 6H2O + light -> C6H12O6 + 6O2. Light reactions occur in thylakoid membranes, producing ATP and NADPH via photophosphorylation while releasing oxygen from photolysis of water. Dark reactions (Calvin Cycle) occur in the stroma, where RuBisCO enzyme fixes CO2 into 3-PGA, synthesizing glyceraldehyde-3-phosphate (G3P)."
            },
            {
                "id": "shiksha_004",
                "title": "Fundamental Calculus Derivatives and Integrals",
                "text": "Standard derivative rules: d/dx(x^n) = n*x^(n-1); d/dx(sin x) = cos x; d/dx(cos x) = -sin x; d/dx(e^x) = e^x; d/dx(ln x) = 1/x. Product rule: (u*v)' = u'v + uv'. Chain rule: d/dx[f(g(x))] = f'(g(x))*g'(x). Fundamental Theorem of Calculus connects differentiation and integration: ∫ from a to b of f(x) dx = F(b) - F(a)."
            }
        ]
    }
}
